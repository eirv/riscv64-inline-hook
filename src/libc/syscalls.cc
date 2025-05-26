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

#include <sys/mman.h>
#include <syscall.h>

#include "libc.h"

namespace rv64hook {

#if defined(__riscv)

void libc_mprotect(const void* addr, size_t size, int prot) {
  register int nr asm("a7") = __NR_mprotect;
  register auto arg0 asm("a0") = addr;
  register auto arg1 asm("a1") = size;
  register auto arg2 asm("a2") = prot;
  asm volatile("ecall" : "=r"(arg0) : "r"(nr), "r"(arg0), "r"(arg1), "r"(arg2));
}

#elif defined(__aarch64__)

void libc_mprotect(const void* addr, size_t size, int prot) {
  register int nr asm("w8") = __NR_mprotect;
  register auto arg0 asm("x0") = addr;
  register auto arg1 asm("x1") = size;
  register auto arg2 asm("x2") = prot;
  asm volatile("svc #0" : "=r"(arg0) : "r"(nr), "r"(arg0), "r"(arg1), "r"(arg2));
}

#else

void libc_mprotect(const void* addr, size_t size, int prot) {
  mprotect(const_cast<void*>(addr), size, prot);
}

#endif

}  // namespace rv64hook
