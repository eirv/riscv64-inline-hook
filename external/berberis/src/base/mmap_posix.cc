/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "berberis/base/mmap.h"

#include <sys/mman.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <random>  // for old versions of GLIBC only (see below)

#include "berberis/base/checks.h"

namespace berberis {

#if defined(__LP64__) && !defined(__x86_64__)
namespace {

// arc4random was introduced in GLIBC 2.36
#if defined(__GLIBC__) && ((__GLIBC__ < 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 36)))
uint32_t arc4random_uniform(uint32_t upper_bound) {
  // Fall back to implementation-defined stl random
  static std::random_device random_device("/dev/urandom");
  static std::mt19937 generator(random_device());
  std::uniform_int_distribution<uint32_t> distrib(0, upper_bound);
  return distrib(generator);
}
#endif

void* TryMmap32Bit(MmapImplArgs args) {
  // Outside of x86_64 mapping in the lower 32bit address space
  // is achieved by trying to map at the random 32bit address with
  // hint and then verifying that the resulted map indeed falls in
  // lower 32bit address space. Note that if another mapping already
  // exists "the kernel picks a new address that may or may not
  // depend on the hint." which makes it more difficult.

  constexpr uintptr_t kMinAddress = 0x10000;

  // This is always positive hence no sign-extend.
  constexpr uintptr_t kMaxAddress = std::numeric_limits<int32_t>::max();

  // This number is somewhat arbitrary. We want it to be big enough so that it
  // doesn't fail prematurely when 2G space has lower availability, but not too
  // big so it doesn't take forever.
  constexpr size_t kMaxMapAttempts = 512;

  static std::atomic<uintptr_t> saved_hint = 0;
  uintptr_t hint = saved_hint.load();

  uintptr_t arc4_random_upper_bound = kMaxAddress - kMinAddress;

  if (args.size == 0) {
    return MAP_FAILED;
  }

  if (__builtin_usubl_overflow(arc4_random_upper_bound, args.size, &arc4_random_upper_bound)) {
    return MAP_FAILED;
  }
  CHECK_LE(arc4_random_upper_bound, kMaxAddress - kMinAddress);

  if (hint == 0 || hint > (arc4_random_upper_bound + kMinAddress)) {
    hint = arc4random_uniform(static_cast<uint32_t>(arc4_random_upper_bound)) + kMinAddress;
  }

  for (size_t i = 0; i < kMaxMapAttempts; i++) {
    // PROT_NONE, MAP_NORESERVE to make it faster since this may take several attempts.
    // We'll do another mmap() with proper flags on top of this one below.
    void* addr = mmap(reinterpret_cast<void*>(hint),
                      args.size,
                      PROT_NONE,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE,
                      0,
                      0);
    if (addr == MAP_FAILED) {
      return MAP_FAILED;
    }

    uintptr_t start = reinterpret_cast<uintptr_t>(addr);
    uintptr_t end = start + args.size;

    if (end <= kMaxAddress) {
      saved_hint.store(AlignUpPageSize(end));  // next hint
      return mmap(addr, args.size, args.prot, MAP_FIXED | args.flags, args.fd, args.offset);
    }

    hint = arc4random_uniform(static_cast<uint32_t>(arc4_random_upper_bound)) + kMinAddress;
  }

  saved_hint.store(0);
  return MAP_FAILED;
}

}  // namespace

#endif  // defined(__LP64__) && !defined(__x86_64__)

void* MmapImpl(MmapImplArgs args) {
  if ((args.berberis_flags & kMmapBerberis32Bit) != 0) {
    // This doesn't make sense for MAP_FIXED
    CHECK_EQ(args.flags & MAP_FIXED, 0);
#if defined(__x86_64__)
    args.flags |= MAP_32BIT;
#elif defined(__LP64__)
    return TryMmap32Bit(args);
#endif
  }
  return mmap(args.addr, args.size, args.prot, args.flags, args.fd, args.offset);
}

void* MmapImplOrDie(MmapImplArgs args) {
  void* ptr = MmapImpl(args);
  CHECK_NE(ptr, MAP_FAILED);
  return ptr;
}

void MunmapOrDie(void* ptr, size_t size) {
  int res = munmap(ptr, size);
  CHECK_EQ(res, 0);
}

void MprotectOrDie(void* ptr, size_t size, int prot) {
  int res = mprotect(ptr, size, prot);
  CHECK_EQ(res, 0);
}

}  // namespace berberis
