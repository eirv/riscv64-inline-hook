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

#pragma once

#include <tuple>

#include "rv64hook.h"

namespace rv64hook {

class ScopedWritableAllocatedMemory;

class Memory {
 public:
  static void* Alloc(size_t size);

  static void* Alloc(size_t size, uintptr_t start, uintptr_t end);

  static void* Realloc(void* ptr, size_t size);

  static void Free(void* ptr);

  static bool Copy(void* addr, const void* src, size_t size);

 private:
  static constexpr const char* kTag = "Memory";

  static Memory* default_allocator_;
  static Memory* root_allocator_;

  Memory* next_{};
  uint8_t* heap_;
  size_t heap_size_;
  uint8_t* chunk_table_;
  size_t allocated_chunks_{};
  size_t references_{};

  Memory(void* heap, size_t heap_size, void* chunk_table)
      : heap_(static_cast<uint8_t*>(heap)),
        heap_size_(heap_size),
        chunk_table_(static_cast<uint8_t*>(chunk_table)) {
  }

  static Memory* NewAllocator(uintptr_t start,
                              uintptr_t end,
                              size_t min_size,
                              size_t recommended_size = 0);

  static int AllocChunk(uint8_t* chunk_table, size_t total_chunks, size_t chunk_count);

  static void* AllocOSMemory(size_t size, void* start = nullptr);

  static void FreeOSMemory(void* ptr, size_t size);

  static void ProtectOSMemory(void* ptr, size_t size, bool writable);

  void* DoAlloc(size_t size);

  void* DoRealloc(void* ptr, size_t size);

  void DoFree(void* ptr);

  ~Memory();

  friend class ScopedWritableAllocatedMemory;
};

class ScopedWritableAllocatedMemory {
 public:
  ScopedWritableAllocatedMemory(void* ptr);

  ScopedWritableAllocatedMemory(Memory* allocator);

  ~ScopedWritableAllocatedMemory();

 private:
  Memory* allocator_ = nullptr;
};

}  // namespace rv64hook
