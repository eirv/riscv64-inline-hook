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

#include "rv64hook.h"

#include <sys/mman.h>
#include <unistd.h>

#include <cstring>

#include "hook_handle.h"
#include "hook_locker.h"
#include "instruction_relocator.h"
#include "logger.h"
#include "memory_allocator.h"
#include "trampoline.h"

namespace rv64hook {

static constexpr const char* kTag = "Hook";

HookHandle* DoHook(func_t address,
                   func_t hook,
                   RegisterHandler pre_handler,
                   RegisterHandler post_handler,
                   void* data,
                   func_t* user_backup_addr) {
  HookLocker locker;
  auto info = HookInfo::Lookup(address);
  if (info) {
    if (info->handle_count == 0xFFFF) [[unlikely]] {
      SET_ERROR("Too many hooks");
      return nullptr;
    }
  } else {
    auto [trampoline, is_user_alloc] = MemoryAllocator::AllocTrampoline(address);
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
    Trampoline::WriteFirstTrampoline(address, trampoline, type);
  }
  return info->NewHookHandle(hook, pre_handler, post_handler, data, user_backup_addr);
}

[[gnu::visibility("default"), maybe_unused]] HookHandle* InlineHook(func_t address,
                                                                    func_t hook,
                                                                    func_t* backup) {
  if (!address || !hook) [[unlikely]] {
    SET_ERROR("Invalid argument");
    return nullptr;
  }
  return DoHook(address, hook, nullptr, nullptr, nullptr, backup);
}

[[gnu::visibility("default"), maybe_unused]] HookHandle* InlineInstrument(
    func_t address,
    RegisterHandler pre_handler,
    RegisterHandler post_handler,
    void* data,
    func_t* backup) {
  if (!address || (!pre_handler && !post_handler)) [[unlikely]] {
    SET_ERROR("Invalid argument");
    return nullptr;
  }
  return DoHook(address, nullptr, pre_handler, post_handler, data, backup);
}

[[gnu::visibility("default"), maybe_unused]] void UnhookFunction(func_t address) {
  HookLocker locker;
  if (!address) [[unlikely]] {
    return;
  }
  auto info = HookInfo::Lookup(address);
  if (info && info->root_handle) [[likely]] {
    info->root_handle->UnhookAll();
  }
}

}  // namespace rv64hook
