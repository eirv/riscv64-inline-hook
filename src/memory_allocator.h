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

class MemoryAllocator {
 public:
  static MemoryAllocator* GetDefault();

  static std::tuple<void*, bool> AllocTrampoline(func_t func);

  static MemoryAllocator* NewAllocator(void* start,
                                       void* end,
                                       size_t min_size,
                                       size_t recommended_size = 0);

  static void* AllocMemory(size_t size, void* start = nullptr);

  static void FreeMemory(void* ptr, size_t size);

  static TrampolineAllocator* GetTrampolineAllocator() {
    return &ta_;
  }

  void* Alloc(size_t size);

  void* Realloc(void* ptr, size_t size);

  void Free(void* ptr);

 private:
  static constexpr const char* kTag = "Memory Allocator";

  static MemoryAllocator* default_allocator_;
  static MemoryAllocator* root_allocator_;

  static TrampolineAllocator ta_;

  MemoryAllocator* next_;
  uint8_t* heap_;
  size_t heap_size_;
  uint8_t* chunk_table_;

  MemoryAllocator(void* heap, size_t heap_size, void* chunk_table)
      : next_(nullptr),
        heap_(static_cast<uint8_t*>(heap)),
        heap_size_(heap_size),
        chunk_table_(static_cast<uint8_t*>(chunk_table)) {
  }

  static bool SetTrampolineAllocator_(TrampolineAllocator& allocator);

  static int AllocChunk(uint8_t* chunk_table, size_t total_chunks, size_t chunk_count);

  friend bool SetTrampolineAllocator(TrampolineAllocator);
};

}  // namespace rv64hook
