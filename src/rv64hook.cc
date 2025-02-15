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

#include <cstring>
#include <map>

#include "arch/common/instruction_relocator.h"
#include "arch/common/trampoline.h"
#include "function_record.h"
#include "hook_handle.h"
#include "hook_locker.h"
#include "logger.h"
#include "memory.h"
#include "rv64hook_internal.h"

namespace rv64hook {

static constexpr const char* kTag = "Hook";

static TrampolineAllocator trampoline_allocator_(TrampolineType::kDefault);
static std::map<func_t, FunctionRecord> function_records_;

HookHandle* DoHook(func_t address,
                   func_t hook,
                   RegisterHandler pre_handler,
                   RegisterHandler post_handler,
                   void* data,
                   func_t* user_backup_addr,
                   uint32_t flags) {
  HookLocker locker;
  ClearError();

  auto info = HookInfo::Lookup(address);
  if (info) {
    if (info->handle_count == 0xFFFF) [[unlikely]] {
      SET_ERROR("Too many hooks");
      return nullptr;
    }
  } else {
    if (uint8_t read_test[32]; !Memory::Copy(read_test, address, sizeof(read_test))) [[unlikely]] {
      SET_ERROR("Function is not readable");
      return nullptr;
    }

    auto [trampoline, is_user_alloc] = Trampoline::AllocSecondTrampoline(address);
    if (!trampoline) {
      return nullptr;
    }
    auto type = Trampoline::GetSuggestedTrampolineType(address, trampoline);

    void* relocated = nullptr;
    auto overwrite_size = InstructionRelocator::Relocate(
        address, Trampoline::GetFirstTrampolineSize(type), &relocated);
    if (overwrite_size == 0) [[unlikely]] {
      return nullptr;
    }

    info = HookInfo::Create(address, trampoline, is_user_alloc, relocated, overwrite_size);
    if (!Trampoline::WriteFirstTrampoline(address, trampoline, type)) [[unlikely]] {
      info->Unhook(false);
      SET_ERROR("Function is not writable");
      return nullptr;
    }
  }
  return info->NewHookHandle(hook, pre_handler, post_handler, data, user_backup_addr);
}

[[gnu::visibility("default"), maybe_unused]] HookHandle* InlineHook(func_t address,
                                                                    func_t hook,
                                                                    func_t* backup,
                                                                    uint32_t flags) {
  if (!address || !hook || flags != 0) [[unlikely]] {
    SET_ERROR("Invalid argument");
    return nullptr;
  }
  return DoHook(address, hook, nullptr, nullptr, nullptr, backup, flags);
}

[[gnu::visibility("default"), maybe_unused]] HookHandle* InlineInstrument(
    func_t address,
    RegisterHandler pre_handler,
    RegisterHandler post_handler,
    void* data,
    func_t* backup,
    uint32_t flags) {
  if (!address || (!pre_handler && !post_handler) || flags != 0) [[unlikely]] {
    SET_ERROR("Invalid argument");
    return nullptr;
  }
  return DoHook(address, nullptr, pre_handler, post_handler, data, backup, flags);
}

[[gnu::visibility("default"), maybe_unused]] bool InlineUnhook(func_t address) {
  if (!address) [[unlikely]] {
    return false;
  }

  HookLocker locker;
  ClearError();

  if (auto info = HookInfo::Lookup(address); info && info->root_handle) [[likely]] {
    info->root_handle->UnhookAllExt();
    return true;
  }

  if (auto it = function_records_.find(address); it != function_records_.end()) {
    it->second.Unhook();
    function_records_.erase(address);
    return true;
  }

  return false;
}

[[gnu::visibility("default"), maybe_unused]] bool WriteTrampoline(func_t address,
                                                                  func_t hook,
                                                                  func_t* backup) {
  if (!address || !hook) [[unlikely]] {
    SET_ERROR("Invalid argument");
    return false;
  }

  HookLocker locker;
  ClearError();

  FunctionRecord* record = nullptr;

  if (auto it = function_records_.find(address); it != function_records_.end()) {
    record = &it->second;
    if (!record->IsModified()) [[unlikely]] {
      SET_ERROR("Too many hooks");
      return false;
    }
  } else {
    record = &function_records_.emplace(address, address).first->second;
  }

  return record->WriteTrampoline(hook, backup);
}

TrampolineAllocator* GetTrampolineAllocator() {
  return &trampoline_allocator_;
}

[[gnu::visibility("default"), maybe_unused]] bool SetTrampolineAllocator(
    TrampolineAllocator allocator) {
  HookLocker locker;
  if (allocator.type == TrampolineType::kCustom) {
    if (allocator.custom_alloc && allocator.custom_free) {
      trampoline_allocator_.type = allocator.type;
      trampoline_allocator_.custom_alloc = allocator.custom_alloc;
      trampoline_allocator_.custom_free = allocator.custom_free;
      trampoline_allocator_.data = allocator.data;
      return true;
    }
  } else if (Trampoline::IsValid(allocator.type)) {
    trampoline_allocator_.type = allocator.type;
    trampoline_allocator_.custom_alloc = nullptr;
    trampoline_allocator_.custom_free = nullptr;
    trampoline_allocator_.data = nullptr;
    return true;
  }
  return false;
}

}  // namespace rv64hook
