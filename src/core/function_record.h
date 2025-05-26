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

#include "arch/common/trampoline.h"
#include "rv64hook.h"

namespace rv64hook {

class FunctionRecord {
 public:
  FunctionRecord(func_t address);

  bool IsModified();

  int WriteTrampoline(func_t hook, func_t* backup);

  void Unhook();

 private:
  func_t address_;
  void* backup_trampoline_;
  int16_t original_function_hash_;
  uint8_t overwrite_size_;
  uint8_t function_backup_[kMaxFirstTrampolineSize];

  static int16_t ComputeHash(func_t address, size_t size);
};

}  // namespace rv64hook
