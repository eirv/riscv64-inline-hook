/**
 * This file is part of riscv64-inline-hook.
 *
 * riscv64-inline-hook is free software: you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * riscv64-inline-hook is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with riscv64-inline-hook.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "misc-no-recursion"

#include "memory.h"

#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

#include "arch/common/trampoline.h"
#include "hook_locker.h"
#include "libc/libc.h"
#include "logger.h"
#include "rv64hook.h"

namespace rv64hook {

struct MemoryHeader {
  uint32_t magic;
  uint32_t chunk_count;
  Memory* allocator;
};

class MemorySpace {
 public:
  uintptr_t address;
  size_t size;

  inline MemorySpace(uintptr_t addr, size_t sz) : address(addr), size(sz) {
  }

  inline bool operator<(const MemorySpace& o) const {
    return size > o.size;
  }

  inline bool operator==(const MemorySpace& o) const {
    return address == o.address;
  }
};

static constexpr uint32_t kMemoryMagic = 0xCAFEBEEF;

static constexpr size_t kHeapSize = 1 * 1024 * 1024;
static constexpr size_t kSmallHeapSize = 64 * 1024;
static constexpr size_t kChunkSize = 128;
static constexpr size_t kAlignment = sizeof(MemoryHeader);

static_assert(kAlignment % sizeof(void*) == 0, "Bad kAlignment");

Memory* Memory::default_allocator_ = nullptr;
Memory* Memory::root_allocator_ = nullptr;

static MemoryHeader* GetMemoryHeader(void* ptr) {
  if (!ptr) [[unlikely]] {
    return nullptr;
  }

  auto header = &reinterpret_cast<MemoryHeader*>(ptr)[-1];
  if (header->magic != kMemoryMagic || header->chunk_count == 0 || !header->allocator)
      [[unlikely]] {
    return nullptr;
  }

  return header;
}

void* Memory::Alloc(size_t size) {
  if (size >= kHeapSize / 2) [[unlikely]] {
    return nullptr;
  }

  for (auto allocator = default_allocator_; allocator; allocator = allocator->next_) {
    auto ptr = allocator->DoAlloc(size);
    if (ptr) return ptr;
  }

  auto heap = AllocOSMemory(kHeapSize);
  auto allocator = new Memory(heap, kHeapSize, malloc(kHeapSize / kChunkSize / 8));
  allocator->next_ = default_allocator_;
  default_allocator_ = allocator;

  return allocator->DoAlloc(size);
}

void* Memory::Alloc(size_t size, uintptr_t start, uintptr_t end) {
  if (size >= kSmallHeapSize / 2) [[unlikely]] {
    return nullptr;
  }

  auto page_size = getpagesize();
  start = __builtin_align_up(start, page_size);
  end = __builtin_align_down(end, page_size);

  for (auto allocator = root_allocator_; allocator; allocator = allocator->next_) {
    auto heap_start = reinterpret_cast<uintptr_t>(allocator->heap_);
    if (heap_start < start) continue;
    if (heap_start + allocator->heap_size_ > end) continue;
    auto ptr = allocator->DoAlloc(size);
    if (ptr) return ptr;
  }

  auto allocator = Memory::NewAllocator(start, end, page_size, kSmallHeapSize);
  if (allocator) {
    allocator->next_ = root_allocator_;
    root_allocator_ = allocator;
    return allocator->DoAlloc(size);
  }

  return nullptr;
}

void* Memory::Realloc(void* ptr, size_t size) {
  auto header = GetMemoryHeader(ptr);
  if (!header) [[unlikely]] {
    return nullptr;
  }

  return header->allocator->DoRealloc(ptr, size);
}

void Memory::Free(void* ptr) {
  auto header = GetMemoryHeader(ptr);
  if (!header) [[unlikely]] {
    return;
  }

  header->allocator->DoFree(ptr);
}

#if !defined(RV64HOOK_USE_PROCESS_VM) || (defined(__ANDROID__) && __ANDROID_API__ < 23)
bool Memory::Copy(void* addr, const void* src, size_t size) {
  libc_memcpy(addr, src, size);
  return true;
}
#else
bool Memory::Copy(void* addr, const void* src, size_t size) {
  iovec in{const_cast<void*>(src), size};
  iovec out{addr, size};
  if (auto r = process_vm_writev(getpid(), &in, 1, &out, 1, 0); r < 0) {
    if (errno == ENOSYS) {
      // QEMU user mode?
      libc_memcpy(addr, src, size);
      return true;
    }
    return false;
  }
  return true;
}
#endif

int Memory::AllocChunk(uint8_t* chunk_table, size_t total_chunks, size_t chunk_count) {
  int start_chunk = -1;
  int count = 0;
  for (int i = 0; i < total_chunks; ++i) {
    int byte_index = i / 8;
    int bit_index = i % 8;
    if (!(chunk_table[byte_index] & (1 << bit_index))) {
      if (start_chunk == -1) {
        start_chunk = i;
      }
      count++;
      if (count == chunk_count) {
        break;
      }
    } else {
      start_chunk = -1;
      count = 0;
    }
  }

  if (count == chunk_count) {
    for (int i = start_chunk; i < start_chunk + chunk_count; ++i) {
      int byte_index = i / 8;
      int bit_index = i % 8;
      chunk_table[byte_index] |= (1 << bit_index);
    }

    return start_chunk;
  } else {
    return -1;
  }
}

void* Memory::DoAlloc(size_t size) {
  [[maybe_unused]] ScopedWritableAllocatedMemory unused(this);
  if (size == 0 || size > heap_size_) [[unlikely]] {
    return nullptr;
  }

  auto expected_chunk_count = __builtin_align_up(size + kAlignment, kChunkSize) / kChunkSize;

  int chunk = AllocChunk(chunk_table_, heap_size_ / kChunkSize, expected_chunk_count);
  if (chunk < 0) return nullptr;

  auto header = reinterpret_cast<MemoryHeader*>(heap_ + (chunk * kChunkSize));
  header->magic = kMemoryMagic;
  header->chunk_count = expected_chunk_count;
  header->allocator = this;
  allocated_chunks_ += expected_chunk_count;
  return header + 1;
}

void* Memory::DoRealloc(void* ptr, size_t size) {
  [[maybe_unused]] ScopedWritableAllocatedMemory unused(this);
  auto header = &reinterpret_cast<MemoryHeader*>(ptr)[-1];

  auto expected_chunk_count = __builtin_align_up(size + kAlignment, kChunkSize) / kChunkSize;
  if (header->chunk_count >= expected_chunk_count) [[unlikely]] {
    return ptr;
  }

  auto start_chunk =
      (reinterpret_cast<uint8_t*>(header) - heap_) / kChunkSize + header->chunk_count;
  auto should_realloc = false;

  for (auto i = start_chunk; i < start_chunk + expected_chunk_count - header->chunk_count; ++i) {
    auto byte_index = i / 8;
    auto bit_index = i % 8;
    if (chunk_table_[byte_index] & (1 << bit_index)) {
      should_realloc = true;
      break;
    }
  }

  if (should_realloc) {
    auto new_ptr = DoAlloc(size);
    if (!new_ptr) return nullptr;
    memcpy(new_ptr, ptr, header->chunk_count * kChunkSize);
    DoFree(ptr);
    return new_ptr;
  } else {
    allocated_chunks_ += expected_chunk_count - header->chunk_count;
    header->chunk_count = expected_chunk_count;
    for (auto i = start_chunk; i < start_chunk + expected_chunk_count - header->chunk_count; ++i) {
      auto byte_index = i / 8;
      auto bit_index = i % 8;
      chunk_table_[byte_index] |= (1 << bit_index);
    }
    return ptr;
  }
}

void Memory::DoFree(void* ptr) {
  [[maybe_unused]] ScopedWritableAllocatedMemory unused(this);
  auto header = &reinterpret_cast<MemoryHeader*>(ptr)[-1];

  auto start_chunk = (reinterpret_cast<uint8_t*>(header) - heap_) / kChunkSize;
  for (auto i = start_chunk; i < start_chunk + header->chunk_count; ++i) {
    auto byte_index = i / 8;
    auto bit_index = i % 8;
    chunk_table_[byte_index] &= ~(1 << bit_index);
  }
  allocated_chunks_ -= header->chunk_count;
  header->chunk_count = 0;

  if (allocated_chunks_ == 0 && this != default_allocator_) [[unlikely]] {
    for (auto allocator = &root_allocator_; *allocator; allocator = &allocator[0]->next_) {
      if (*allocator != this) continue;
      *allocator = next_;
      break;
    }
    FreeOSMemory(heap_, heap_size_);
    delete this;
  }
}

Memory::~Memory() {
}

Memory* Memory::NewAllocator(uintptr_t start,
                             uintptr_t end,
                             size_t min_size,
                             size_t recommended_size) {
  auto maps = fopen("/proc/self/maps", "r");
  char* buf = nullptr;
  size_t len;

  uintptr_t previous_end = 0x8000;
  uintptr_t mem_start, mem_end;
  std::set<MemorySpace> spaces;
  std::set<MemorySpace> dangerous_spaces;

  for (char* tmp; getline(&buf, &len, maps) != -1; previous_end = mem_end) {
    mem_start = strtoul(buf, &tmp, 16);
    mem_end = strtoul(tmp + 1, &tmp, 16);
    if (previous_end < start) continue;
    auto size = (mem_start > end ? mem_start : end) - previous_end;
    if (size < min_size) continue;
    spaces.emplace(previous_end, size);
    if (tmp[1] == '-' && tmp[2] == '-' && tmp[3] == '-' && tmp[4] == 'p') {
      dangerous_spaces.emplace(mem_start, mem_end - mem_start);
    }
  }

  free(buf);
  fclose(maps);

  auto new_memory = [=](const std::set<MemorySpace>& memory_spaces) -> Memory* {
    for (const auto& space : memory_spaces) {
      if (space.address > end) continue;
      auto size = std::min(space.size, recommended_size);
      auto base = space.address + std::min(space.size, end - space.address) - size;
      if (base < start || base + size > end) {
        continue;
      }
      if (uint8_t vec = 0;
          mincore(reinterpret_cast<void*>(base), getpagesize(), &vec) == 0 && vec != 0) {
        continue;
      }
      auto heap = AllocOSMemory(size, reinterpret_cast<void*>(base));
      if (!heap) [[unlikely]] {
        continue;
      }
      return new Memory(heap, size, malloc(size / kChunkSize / 8));
    }
    return nullptr;
  };

  auto memory = new_memory(spaces);
  return memory ? memory : new_memory(dangerous_spaces);
}

void* Memory::AllocOSMemory(size_t size, void* start) {
  auto ptr =
      mmap(start,
           size,
           // Make sure it can mmap as rwx
           PROT_READ | PROT_WRITE | PROT_EXEC,
           start ? MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE : MAP_PRIVATE | MAP_ANONYMOUS,
           -1,
           0);
  if (ptr == MAP_FAILED) return nullptr;
  if (mprotect(ptr, size, PROT_READ | PROT_EXEC) != 0) {
    munmap(ptr, size);
    return nullptr;
  }
  // prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ptr, size, "heap");
  return ptr;
}

void Memory::FreeOSMemory(void* ptr, size_t size) {
  munmap(ptr, size);
}

void Memory::ProtectOSMemory(void* ptr, size_t size, bool writable) {
  libc_mprotect(ptr, size, writable ? PROT_READ | PROT_WRITE | PROT_EXEC : PROT_READ | PROT_EXEC);
}

ScopedWritableAllocatedMemory::ScopedWritableAllocatedMemory(void* ptr) {
  auto header = GetMemoryHeader(ptr);
  if (!header) [[unlikely]] {
    return;
  }

  allocator_ = header->allocator;
  if (++(allocator_->references_) == 1) {
    Memory::ProtectOSMemory(allocator_->heap_, allocator_->heap_size_, true);
  }
}

ScopedWritableAllocatedMemory::ScopedWritableAllocatedMemory(Memory* allocator)
    : allocator_(allocator) {
  if (++(allocator_->references_) == 1) {
    Memory::ProtectOSMemory(allocator_->heap_, allocator_->heap_size_, true);
  }
}

ScopedWritableAllocatedMemory::~ScopedWritableAllocatedMemory() {
  if (allocator_ && --(allocator_->references_) == 0) {
    Memory::ProtectOSMemory(allocator_->heap_, allocator_->heap_size_, false);
  }
}

}  // namespace rv64hook

#pragma clang diagnostic pop
