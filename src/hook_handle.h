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

#include <map>

#include "arch/common/trampoline.h"
#include "rv64hook_internal.h"

namespace rv64hook {

class HookHandleExt;

class HookInfo {
 public:
  func_t address;
  HookHandleExt* root_handle;
  void* trampoline;
  decltype(TrampolineAllocator::custom_free) custom_free;
  void* custom_data;
  void* relocated;
  uint16_t handle_count;
  uint8_t function_backup_size;
  uint8_t function_backup[kMaxBackupSize];

  static HookInfo* Lookup(func_t func);

  static HookInfo* Create(func_t address,
                          void* trampoline,
                          bool is_user_alloc,
                          void* relocated,
                          uint8_t function_backup_size);

  HookHandleExt* NewHookHandle(func_t hook,
                               RegisterHandler pre_handler,
                               RegisterHandler post_handler,
                               void* data,
                               func_t* user_backup_addr);

  [[nodiscard]] TrampolineData* GetTrampolineData() const;

  void Unhook(bool initialized = true);

 private:
  static std::map<func_t, HookInfo> hooks_;
};

class HookHandleExt : public HookHandle {
 public:
  HookHandleExt(func_t address,
                func_t hook,
                RegisterHandler pre_handler,
                RegisterHandler post_handler,
                void* data,
                func_t* user_backup_addr);

  bool SetEnabledExt(bool enabled);

  void UpdateBackup(func_t new_backup);

  void UnhookExt();

  void UnhookAllExt();

 private:
  HookHandleExt* previous_;
  [[maybe_unused]] HookHandleExt* next_;
  func_t hook_;
  [[maybe_unused]] RegisterHandler pre_handler_;
  [[maybe_unused]] RegisterHandler post_handler_;
  [[maybe_unused]] void* data_;
  func_t* user_backup_addr_;
  bool enabled_;

  friend class HookInfo;
};

}  // namespace rv64hook
