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
#include <unistd.h>

#include "arch/common/trampoline.h"
#include "rv64hook.h"

namespace rv64hook {

[[gnu::visibility("default"), maybe_unused]] ScopedRWXMemory::ScopedRWXMemory(void* address,
                                                                              int original_prot)
    : prot_(original_prot) {
  if (!address) {
    address_ = nullptr;
    return;
  }
  auto page_size = getpagesize();
  address_ = __builtin_align_down(address, page_size);
  size_ = __builtin_align_up(reinterpret_cast<uintptr_t>(address) + kMaxBackupSize, page_size) -
          reinterpret_cast<uintptr_t>(address_);
  if (mprotect(address_, size_, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
    address_ = nullptr;
  }
}

[[gnu::visibility("default"), maybe_unused]] ScopedRWXMemory::~ScopedRWXMemory() {
  if (address_) mprotect(address_, size_, prot_);
}

}  // namespace rv64hook
