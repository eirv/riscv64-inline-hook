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

extern "C" {
void __rv64hook_libc_memcpy_vext(void* dest, const void* src, size_t count);
}

namespace rv64hook {

void __rv64hook_libc_memcpy_gc(void* dest, const void* src, size_t count);

static inline void libc_memcpy(void* dest, const void* src, size_t count) {
#if defined(__riscv) && defined(__riscv_vector)
  __rv64hook_libc_memcpy_vext(dest, src, count);
#else
  __rv64hook_libc_memcpy_gc(dest, src, count);
#endif
}

}  // namespace rv64hook
