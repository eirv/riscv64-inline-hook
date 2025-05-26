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

#include <pthread.h>

#include <tuple>

#include "core/rv64hook_internal.h"

namespace rv64hook {

#if defined(__riscv)
static constexpr uint8_t kMaxFirstTrampolineSize = 28;
#elif defined(__aarch64__)
static constexpr uint8_t kMaxFirstTrampolineSize = 48;
#endif

class HookHandleExt;

struct TrampolineData {
  [[maybe_unused]] HookHandleExt* root_handle;
  [[maybe_unused]] void* hook;
  [[maybe_unused]] void* backup;
  [[maybe_unused]] void* (*getspecific)(pthread_key_t);
  [[maybe_unused]] int (*setspecific)(pthread_key_t, const void*);
  [[maybe_unused]] pthread_key_t tls_key;
  [[maybe_unused]] uint16_t post_handlers;
  [[maybe_unused]] bool enabled;
};

static_assert(sizeof(pthread_key_t) == sizeof(int), "Bad pthread_key_t");

class Trampoline {
 public:
  static bool IsValid(TrampolineType type);

  static TrampolineType GetSuggestedTrampolineType(func_t address, void* target);

  static int GetFirstTrampolineSize(TrampolineType type);

  static bool WriteFirstTrampoline(func_t address, void* target, TrampolineType type);

  static std::tuple<void*, bool> AllocSecondTrampoline(func_t address);

  static TrampolineData* GetTrampolineData(void* trampoline);

  [[gnu::always_inline]] static std::tuple<const void*, size_t> GetSecondTrampoline();

 private:
  static constexpr const char* kTag = "Trampoline";

#ifdef __riscv
  static bool Write32BitJumpInstruction(uint32_t op, func_t address, uint32_t v);
#endif
};

}  // namespace rv64hook
