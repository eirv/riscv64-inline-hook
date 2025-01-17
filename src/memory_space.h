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

#include <cstdint>

namespace rv64hook {

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

}  // namespace rv64hook
