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

#if __has_include("dobby.h")
#include "dobby.h"
#else

#include "rv64hook.h"

#define RT_FAILED  -1
#define RT_SUCCESS 0

#define install_hook_name(name, fn_ret_t, fn_args_t...)             \
  static fn_ret_t fake_##name(fn_args_t);                           \
  static fn_ret_t (*orig_##name)(fn_args_t);                        \
  static void install_hook_##name(void* sym_addr) {                 \
    DobbyHook(sym_addr,                                             \
              reinterpret_cast<dobby_dummy_func_t>(fake_##name),    \
              reinterpret_cast<dobby_dummy_func_t*>(&orig_##name)); \
  }                                                                 \
  fn_ret_t fake_##name(fn_args_t)

typedef void* dobby_dummy_func_t;

typedef union _FPReg {
  struct {
    double d1;
  } d;
  struct {
    float f1;
    float f2;
  } f;
} FPReg;

typedef struct {
  union {
    struct {
      uint64_t ra, sp, gp, tp, t0, t1, t2, s0, s1, a0, a1, a2, a3, a4, a5, a6, a7, s2, s3, s4, s5,
          s6, s7, s8, s9, s10, s11, t3, t4, t5, t6;
    } regs;
  } general;

  union {
    FPReg f[32];
    struct {
      FPReg ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7, fs0, fs1, fa0, fa1, fa2, fa3, fa4, fa5, fa6,
          fa7, fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11, ft8, ft9, ft10, ft11;
    } regs;
  } floating;

  bool return_early;
} DobbyRegisterContext;

template <typename DobbyDummyFunc>
static inline int DobbyHook(void* address,
                            DobbyDummyFunc replace_func,
                            DobbyDummyFunc* origin_func) {
  rv64hook::ScopedRWXMemory unused(address);
  auto handle = rv64hook::InlineHook(
      address, reinterpret_cast<void*>(replace_func), reinterpret_cast<void**>(origin_func));
  return handle ? RT_SUCCESS : RT_FAILED;
}

typedef void (*dobby_instrument_callback_t)(void* address, DobbyRegisterContext* ctx);
static inline int DobbyInstrument(void* address, dobby_instrument_callback_t pre_handler) {
  rv64hook::ScopedRWXMemory unused(address);
  auto handle = rv64hook::InlineInstrument(
      address,
      [](auto ctx, auto handle, auto data) {
        reinterpret_cast<dobby_instrument_callback_t>(data)(
            handle->GetAddress(), reinterpret_cast<DobbyRegisterContext*>(ctx));
      },
      nullptr,
      reinterpret_cast<void*>(pre_handler));
  return handle ? RT_SUCCESS : RT_FAILED;
}

static inline int DobbyDestroy(void* address) {
  rv64hook::ScopedRWXMemory unused(address);
  return rv64hook::InlineUnhook(address) ? RT_SUCCESS : RT_FAILED;
}

static inline const char* DobbyGetVersion() {
  return "1.0";
}

static inline void dobby_enable_near_branch_trampoline() {
  rv64hook::SetTrampolineAllocator(rv64hook::TrampolineType::kDefault);
}

static inline void dobby_disable_near_branch_trampoline() {
  rv64hook::SetTrampolineAllocator(rv64hook::TrampolineType::kWide);
}

#ifndef __cplusplus
#error unsupported
#endif

#endif
