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
#pragma ide diagnostic ignored "misc-no-recursion"

#include "memory_allocator.h"

#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

#include "hook_locker.h"
#include "logger.h"
#include "memory_space.h"
#include "trampoline.h"

namespace rv64hook {

struct MemoryHeader {
  MemoryAllocator* allocator;
  uint16_t chunk_count;
};

static constexpr size_t kHeapSize = 1 * 1024 * 1024;
static constexpr size_t kSmallHeapSize = 64 * 1024;
static constexpr size_t kChunkSize = 128;
static constexpr size_t kAlignment = sizeof(MemoryHeader);

MemoryAllocator* MemoryAllocator::default_allocator_ = nullptr;
MemoryAllocator* MemoryAllocator::root_allocator_ = nullptr;
TrampolineAllocator MemoryAllocator::ta_(TrampolineType::kDefault);

MemoryAllocator* MemoryAllocator::GetDefault() {
  if (default_allocator_) return default_allocator_;
  auto heap = AllocMemory(kHeapSize, (void*)0x100000);
  auto allocator = new MemoryAllocator(heap, kHeapSize, malloc(kHeapSize / kChunkSize / 8));
  default_allocator_ = allocator;
  return allocator;
}

std::tuple<void*, bool> MemoryAllocator::AllocTrampoline(func_t func) {
  auto size = sizeof(kTrampoline) + sizeof(TrampolineData);
  uintptr_t start = 0, end = 0;
  bool is_user_alloc = false;
  void* trampoline = nullptr;
  switch (ta_.type) {
    case TrampolineType::kPC20:
      start = reinterpret_cast<uintptr_t>(func) - 0xFFFFE;
      end = reinterpret_cast<uintptr_t>(func) + 0xFFFFE;
      break;
    case TrampolineType::kPC32:
      start = reinterpret_cast<uintptr_t>(func);
      end = reinterpret_cast<uintptr_t>(func) + 0xFFFFF800;
      break;
    case TrampolineType::kVA32:
      end = 0xFFFFF800;
      break;
    case TrampolineType::kCustom:
      trampoline = ta_.custom_alloc(size, ta_.data);
      is_user_alloc = trampoline != nullptr;
      break;
    default:
      break;
  }
  if (!trampoline) {
    if (start || end) {
      for (auto allocator = root_allocator_; allocator; allocator = allocator->next_) {
        auto heap_start = reinterpret_cast<uintptr_t>(allocator->heap_);
        if (heap_start < start) continue;
        if (heap_start + allocator->heap_size_ > end) continue;
        trampoline = allocator->Alloc(size);
        if (trampoline) break;
      }
      if (!trampoline) {
        auto allocator = NewAllocator(reinterpret_cast<void*>(start),
                                      reinterpret_cast<void*>(end),
                                      getpagesize(),
                                      kSmallHeapSize);
        if (!allocator) [[unlikely]] {
          allocator = GetDefault();
        }
        trampoline = allocator->Alloc(size);
      }
    } else {
      trampoline = GetDefault()->Alloc(size);
    }
    if (!trampoline) [[unlikely]] {
      return {};
    }
  }
  memcpy(trampoline, kTrampoline, sizeof(kTrampoline));
  memset(static_cast<char*>(trampoline) + sizeof(kTrampoline), 0, sizeof(TrampolineData));
  __builtin___clear_cache(static_cast<char*>(trampoline),
                          static_cast<char*>(trampoline) + sizeof(kTrampoline));
  return {trampoline, is_user_alloc};
}

int MemoryAllocator::AllocChunk(uint8_t* chunk_table, size_t total_chunks, size_t chunk_count) {
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

void* MemoryAllocator::Alloc(size_t size) {
  if (size == 0 || size > heap_size_) [[unlikely]] {
    return nullptr;
  }

  auto expected_chunk_count = __builtin_align_up(size + kAlignment, kChunkSize) / kChunkSize;

  int chunk = AllocChunk(chunk_table_, heap_size_ / kChunkSize, expected_chunk_count);
  if (chunk < 0) return nullptr;

  auto header = reinterpret_cast<MemoryHeader*>(heap_ + (chunk * kChunkSize));
  header->allocator = this;
  header->chunk_count = expected_chunk_count;
  return header + 1;
}

void* MemoryAllocator::Realloc(void* ptr, size_t size) {
  if (!ptr) [[unlikely]] {
    return Alloc(size);
  }

  if (size == 0 || size > heap_size_) [[unlikely]] {
    return nullptr;
  }

  auto header = &reinterpret_cast<MemoryHeader*>(ptr)[-1];
  if (header->chunk_count == 0) [[unlikely]] {
    return nullptr;
  }

  if (header->allocator != this) [[unlikely]] {
    return header->allocator->Realloc(ptr, size);
  }

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
    auto new_ptr = Alloc(size);
    if (!new_ptr) return nullptr;
    memcpy(new_ptr, ptr, header->chunk_count * kChunkSize);
    Free(ptr);
    return new_ptr;
  } else {
    header->chunk_count = expected_chunk_count;
    for (auto i = start_chunk; i < start_chunk + expected_chunk_count - header->chunk_count; ++i) {
      auto byte_index = i / 8;
      auto bit_index = i % 8;
      chunk_table_[byte_index] |= (1 << bit_index);
    }
    return ptr;
  }
}

void MemoryAllocator::Free(void* ptr) {
  if (!ptr) [[unlikely]] {
    return;
  }

  auto header = &reinterpret_cast<MemoryHeader*>(ptr)[-1];
  if (header->chunk_count == 0) [[unlikely]] {
    return;
  }

  if (header->allocator != this) [[unlikely]] {
    header->allocator->Free(ptr);
    return;
  }

  auto start_chunk = (reinterpret_cast<uint8_t*>(header) - heap_) / kChunkSize;
  for (auto i = start_chunk; i < start_chunk + header->chunk_count; ++i) {
    auto byte_index = i / 8;
    auto bit_index = i % 8;
    chunk_table_[byte_index] &= ~(1 << bit_index);
  }
  header->chunk_count = 0;
}

MemoryAllocator* MemoryAllocator::NewAllocator(void* start,
                                               void* end,
                                               size_t min_size,
                                               size_t recommended_size) {
  auto maps = fopen("/proc/self/maps", "r");
  char* buf = nullptr;
  size_t len;
  uintptr_t previous_end = 0x8000;
  uintptr_t mem_start, mem_end;
  std::set<MemorySpace> spaces;

  for (char* tmp; getline(&buf, &len, maps) != -1; previous_end = mem_end) {
    mem_start = strtoul(buf, &tmp, 16);
    mem_end = strtoul(tmp + 1, nullptr, 16);
    if (previous_end < reinterpret_cast<uintptr_t>(start)) continue;
    auto size = (mem_start > reinterpret_cast<uintptr_t>(end) ? mem_start :
                                                                reinterpret_cast<uintptr_t>(end)) -
                previous_end;
    if (size < min_size) continue;
    spaces.emplace(previous_end, size);
  }

  free(buf);
  fclose(maps);

  for (const auto& space : spaces) {
    auto size = std::min(space.size, recommended_size);
    auto heap = AllocMemory(size, reinterpret_cast<void*>(space.address + space.size - size));
    if (!heap) [[unlikely]] {
      continue;
    }
    return new MemoryAllocator(heap, size, malloc(size / kChunkSize / 8));
  }

  return nullptr;
}

void* MemoryAllocator::AllocMemory(size_t size, void* start) {
  auto ptr =
      mmap(start,
           size,
           PROT_READ | PROT_WRITE | PROT_EXEC,
           start ? MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE : MAP_PRIVATE | MAP_ANONYMOUS,
           -1,
           0);
  if (ptr == MAP_FAILED) return nullptr;
  prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ptr, size, "heap");
  return ptr;
}

void MemoryAllocator::FreeMemory(void* ptr, size_t size) {
  munmap(ptr, size);
}

bool MemoryAllocator::SetTrampolineAllocator_(TrampolineAllocator& allocator) {
  switch (allocator.type) {
    case TrampolineType::kPC20:
    case TrampolineType::kPC32:
    case TrampolineType::kVA32:
    case TrampolineType::kWide:
      ta_.type = allocator.type;
      ta_.custom_alloc = nullptr;
      ta_.custom_free = nullptr;
      ta_.data = nullptr;
      return true;
    case TrampolineType::kCustom:
      if (allocator.custom_alloc && allocator.custom_free) {
        ta_.type = allocator.type;
        ta_.custom_alloc = allocator.custom_alloc;
        ta_.custom_free = allocator.custom_free;
        ta_.data = allocator.data;
        return true;
      }
      break;
  }
  return false;
}

[[gnu::visibility("default"), maybe_unused]] bool SetTrampolineAllocator(
    TrampolineAllocator allocator) {
  HookLocker locker;
  return MemoryAllocator::SetTrampolineAllocator_(allocator);
}

}  // namespace rv64hook

#pragma clang diagnostic pop
