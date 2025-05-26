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

#include "hook_handle.h"

#include <cstring>

#include "config.h"
#include "hook_locker.h"
#include "logger.h"
#include "memory.h"

namespace rv64hook {

std::map<func_t, HookInfo*> HookInfo::hooks_;

HookInfo* HookInfo::Lookup(func_t func) {
  auto p = hooks_.find(func);
  return p != hooks_.end() ? p->second : nullptr;
}

HookInfo* HookInfo::Create(func_t address,
                           void* trampoline,
                           bool is_user_alloc,
                           void* relocated,
                           uint8_t function_backup_size) {
  auto info = new HookInfo;
  info->address = address;
  info->root_handle = nullptr;
  info->trampoline = trampoline;
  if (is_user_alloc) {
    auto ta = GetTrampolineAllocator();
    info->custom_free = ta->custom_free;
    info->custom_data = ta->data;
  } else {
    info->custom_free = nullptr;
  }
  info->relocated = relocated;
  info->handle_count = 0;
  info->function_backup_size = function_backup_size;
  Memory::Copy(info->function_backup, address, function_backup_size);
  hooks_.emplace(address, info);
  return info;
}

HookHandleExt* HookInfo::NewHookHandle(func_t hook,
                                       RegisterHandler pre_handler,
                                       RegisterHandler post_handler,
                                       void* data,
                                       func_t* user_backup_addr) {
  ScopedWritableAllocatedMemory unused(custom_free ? nullptr : trampoline);

  auto td = GetTrampolineData();
  auto new_handle =
      new HookHandleExt(this, address, hook, pre_handler, post_handler, data, user_backup_addr);

  handle_count++;

  if (root_handle) {
    auto backup = relocated;
    for (auto handle = root_handle; handle; handle = handle->next_) {
      if (handle->hook_) {
        backup = handle->hook_;
      }
      if (!handle->next_) {
        handle->next_ = new_handle;
        new_handle->previous_ = handle;
        break;
      }
    }

    new_handle->backup_ = backup;

    if (user_backup_addr) {
      *user_backup_addr = backup;
    }

    if (hook) {
      if (td->hook) td->hook = hook;
      else td->backup = hook;
    } else {
      td->hook = nullptr;
      td->backup = backup;
      if (post_handler) td->post_handlers++;
    }
  } else {
    root_handle = new_handle;
    td->root_handle = new_handle;
    new_handle->backup_ = relocated;

    if (STORE_RETURN_ADDRESS_BY_TLS && pthread_key_create(&td->tls_key, nullptr) == 0) {
      td->getspecific = pthread_getspecific;
      td->setspecific = pthread_setspecific;
    }

    if (user_backup_addr) {
      *user_backup_addr = relocated;
    }

    if (hook) {
      td->hook = hook;
    } else {
      td->backup = relocated;
      if (post_handler) td->post_handlers = 1;
    }

    td->enabled = true;
  }

  return new_handle;
}

TrampolineData* HookInfo::GetTrampolineData() const {
  return Trampoline::GetTrampolineData(trampoline);
}

void HookInfo::Unhook(bool initialized) {
  if (initialized) {
    Memory::Copy(address, function_backup, function_backup_size);
    __builtin___clear_cache(static_cast<char*>(address),
                            static_cast<char*>(address) + function_backup_size);
  }

  if (auto td = GetTrampolineData(); td->getspecific) {
    pthread_key_delete(td->tls_key);
  }

  if (custom_free) {
    custom_free(trampoline, custom_data);
  } else {
    Memory::Free(trampoline);
  }
  Memory::Free(relocated);
  hooks_.erase(this);
}

HookHandleExt::HookHandleExt(HookInfo* info,
                             func_t address,
                             func_t hook,
                             RegisterHandler pre_handler,
                             RegisterHandler post_handler,
                             void* data,
                             func_t* user_backup_addr)
    : HookHandle(),
      info_(info),
      previous_(nullptr),
      next_(nullptr),
      hook_(hook),
      pre_handler_(pre_handler),
      post_handler_(post_handler),
      data_(data),
      user_backup_addr_(user_backup_addr),
      enabled_(true) {
  address_ = address;
}

bool HookHandleExt::SetEnabledExt(bool enabled) {
  auto old = enabled_;
  enabled_ = enabled;
  return old;
}

bool HookHandleExt::SetEnabledAllExt(bool enabled) {
  auto info = info_;
  if (!info) [[unlikely]]
    return false;

  ScopedWritableAllocatedMemory unused(info->custom_free ? nullptr : info->trampoline);
  auto td = info->GetTrampolineData();
  auto old = td->enabled;
  td->enabled = enabled;
  return old;
}

void HookHandleExt::UpdateBackup(func_t new_backup) {
  if (user_backup_addr_) [[likely]] {
    if (*user_backup_addr_ == backup_) [[likely]] {
      *user_backup_addr_ = new_backup;
    } else {
      user_backup_addr_ = nullptr;
    }
  }
  backup_ = new_backup;
}

bool HookHandleExt::UnhookExt() {
  auto info = info_;
  if (!info) [[unlikely]] {
    return false;
  }

  if (info->handle_count == 1) {
    delete this;
    info->Unhook();
    return true;
  } else info->handle_count--;

  ScopedWritableAllocatedMemory unused(info->custom_free ? nullptr : info->trampoline);
  auto td = info->GetTrampolineData();

  if (post_handler_) {
    td->post_handlers--;
  }
  if (info->root_handle == this) {
    info->root_handle = next_;
    td->root_handle = next_;
  }
  func_t previous_hook = nullptr;
  for (auto handle = previous_; handle; handle = handle->previous_) {
    if (!handle->hook_) continue;
    previous_hook = handle->hook_;
    break;
  }
  auto new_backup = previous_hook ? previous_hook : info->relocated;
  for (auto handle = next_; handle; handle = handle->next_) {
    handle->UpdateBackup(new_backup);
    if (handle->hook_) break;
  }
  if (hook_) {
    if (td->hook == hook_) {
      td->hook = previous_hook;
    }
    if (td->backup == hook_) {
      td->backup = new_backup;
    }
  }
  if (previous_) {
    previous_->next_ = next_;
  }
  info_ = nullptr;
  delete this;
  return true;
}

bool HookHandleExt::UnhookAllExt() {
  auto info = info_;
  if (!info) [[unlikely]] {
    return false;
  }

  for (auto handle = info->root_handle;;) {
    auto next = handle->next_;
    delete handle;
    if (!next) {
      info->Unhook();
      break;
    }
    handle = next;
  }
  return true;
}

[[gnu::visibility("default"), maybe_unused]] bool HookHandle::SetEnabled(bool enabled) {
  return reinterpret_cast<HookHandleExt*>(this)->SetEnabledExt(enabled);
}

[[gnu::visibility("default"), maybe_unused]] bool HookHandle::SetEnabledAll(bool enabled) {
  return reinterpret_cast<HookHandleExt*>(this)->SetEnabledAllExt(enabled);
}

[[gnu::visibility("default"), maybe_unused]] bool HookHandle::Unhook() {
  HookLocker locker;
  ClearError();
  return reinterpret_cast<HookHandleExt*>(this)->UnhookExt();
}

[[gnu::visibility("default"), maybe_unused]] bool HookHandle::UnhookAll() {
  HookLocker locker;
  ClearError();
  return reinterpret_cast<HookHandleExt*>(this)->UnhookAllExt();
}

}  // namespace rv64hook
