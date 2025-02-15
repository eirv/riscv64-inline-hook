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

#ifdef __cplusplus
#include <cstdint>
#include <type_traits>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef __cplusplus

namespace rv64hook {

typedef void* func_t;
typedef unsigned long reg_t;

class HookHandle {
 public:
  [[nodiscard]] inline func_t GetAddress() const;

  [[nodiscard]] inline func_t GetBackup() const;

  bool SetEnabled(bool enabled);

  void Unhook();

  void UnhookAll();

 protected:
  func_t address_;
  func_t backup_;
};

union freg_t {
  float f;
  double d;
  reg_t raw;

  constexpr inline operator float() const;

  constexpr inline operator double() const;

  constexpr inline operator reg_t() const;

  constexpr inline freg_t& operator=(const freg_t& x);

  constexpr inline freg_t& operator=(float x);

  constexpr inline freg_t& operator=(double x);

  constexpr inline freg_t& operator=(reg_t x);
};

class RegisterContext {
 public:
  reg_t ra, sp, gp, tp;
  reg_t t0, t1, t2;
  reg_t s0, fp, s1;
  reg_t a0, a1, a2, a3, a4, a5, a6, a7;
  reg_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
  reg_t t3, t4, t5, t6;

  freg_t ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7;
  freg_t fs0, fs1;
  freg_t fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7;
  freg_t fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11;
  freg_t ft8, ft9, ft10, ft11;

  template <typename T = reg_t>
  [[nodiscard]] inline T GetReturnValue() const;

  template <typename T>
  inline void ReturnValue(T value);

  inline void ReturnVoid();

  template <uint16_t N, typename T = reg_t>
  [[nodiscard]] inline T GetArg() const;

  template <uint16_t N, typename T>
  inline void SetArg(T value);

 private:
  [[maybe_unused]] bool return_early_;

  template <typename T, typename V>
  static constexpr inline T force_cast(V value);

  template <typename T>
  static constexpr inline T get_reg(reg_t r, freg_t fr);

  template <typename T>
  static constexpr inline void set_reg(reg_t& r, freg_t& fr, T v);
};

typedef void (*RegisterHandler)(RegisterContext* ctx, HookHandle* handle, void* data);

enum class TrampolineType {
  kCustom = 0,
  // 占用被hook函数头 18~20 字节, 跳板分配在 ∞
  kWide = 1,
  // 占用被hook函数头 4 字节, 跳板分配在 PC +- 0~0xF'FFFF
  kPC20 = 2,
  // 占用被hook函数头 8 字节, 跳板分配在 PC + 0~0xFFFF'FFFF
  kPC32 = 3,
  // 占用被hook函数头 8 字节, 跳板分配在 0~0xFFFF'FFFF
  kVA32 = 4,
  kDefault = kPC32,
};

class TrampolineAllocator {
 public:
  TrampolineType type;

  void* (*custom_alloc)(void* function_address, size_t size, void* data);
  void (*custom_free)(void* mem, void* data);
  void* data;

  inline TrampolineAllocator(TrampolineType type = TrampolineType::kDefault);

  inline TrampolineAllocator(decltype(custom_alloc) alloc,
                             decltype(custom_free) free,
                             void* data = nullptr);
};

// ========================= Functions =========================

HookHandle* InlineHook(func_t address, func_t hook, func_t* backup = nullptr, uint32_t flags = 0);

HookHandle* InlineInstrument(func_t address,
                             RegisterHandler pre_handler,
                             RegisterHandler post_handler,
                             void* data = nullptr,
                             func_t* backup = nullptr,
                             uint32_t flags = 0);

int WriteTrampoline(func_t address, func_t hook, func_t* backup = nullptr);

bool InlineUnhook(func_t address);

bool SetTrampolineAllocator(TrampolineAllocator allocator);

[[nodiscard]] const char* GetLastError();

// ========================= Templates =========================

template <typename Func, typename MayLambda = Func>
static inline auto InlineHook(Func address,
                              MayLambda hook,
                              Func* backup = nullptr,
                              uint32_t flags = 0) {
  return InlineHook(reinterpret_cast<func_t>(address),
                    reinterpret_cast<func_t>(static_cast<Func>(hook)),
                    reinterpret_cast<func_t*>(backup),
                    flags);
}

template <typename Data>
struct InstrumentCallbacks {
  void (*pre)(RegisterContext* ctx, HookHandle* handle, Data* data);
  void (*post)(RegisterContext* ctx, HookHandle* handle, Data* data);
};

template <typename Data, typename Func>
static inline auto InlineInstrument(Func address,
                                    InstrumentCallbacks<Data> callbacks,
                                    Data* data = static_cast<void*>(nullptr),
                                    Func* backup = nullptr,
                                    uint32_t flags = 0) {
  return InlineInstrument(reinterpret_cast<func_t>(address),
                          reinterpret_cast<RegisterHandler>(callbacks.pre),
                          reinterpret_cast<RegisterHandler>(callbacks.post),
                          static_cast<void*>(data),
                          reinterpret_cast<func_t*>(backup),
                          flags);
}

template <typename Func, typename MayLambda = Func>
static inline auto WriteTrampoline(Func address, MayLambda hook, Func* backup = nullptr) {
  return WriteTrampoline(reinterpret_cast<func_t>(address),
                         reinterpret_cast<func_t>(static_cast<Func>(hook)),
                         reinterpret_cast<func_t*>(backup));
}

// ========================= Helpers =========================

class ScopedRWXMemory {
 public:
  static constexpr int kRead = 0x1;
  static constexpr int kWrite = 0x2;
  static constexpr int kExec = 0x4;

  template <typename Func>
  inline explicit ScopedRWXMemory(Func address);

  ScopedRWXMemory(func_t address, int original_prot);

  [[nodiscard]] inline bool IsValid();

  ~ScopedRWXMemory();

 private:
  func_t address_;
  uint32_t size_;
  int prot_;
};

// ==================================================

inline func_t HookHandle::GetAddress() const {
  return address_;
}

inline func_t HookHandle::GetBackup() const {
  return backup_;
}

constexpr inline bool operator==(const freg_t& a, const freg_t& b) {
  return a.raw == b.raw;
}

constexpr inline freg_t::operator float() const {
  return f;
}

constexpr inline freg_t::operator double() const {
  return d;
}

constexpr inline freg_t::operator reg_t() const {
  return raw;
}

constexpr inline freg_t& freg_t::operator=(const freg_t& x) {
  raw = x.raw;
  return *this;
}

constexpr inline freg_t& freg_t::operator=(float x) {
  f = x;
  return *this;
}

constexpr inline freg_t& freg_t::operator=(double x) {
  d = x;
  return *this;
}

constexpr inline freg_t& freg_t::operator=(reg_t x) {
  raw = x;
  return *this;
}

template <typename T>
inline T RegisterContext::GetReturnValue() const {
  return GetArg<0, T>();
}

template <typename T>
inline void RegisterContext::ReturnValue(T value) {
  SetArg<0>(value);
  ReturnVoid();
}

inline void RegisterContext::ReturnVoid() {
  return_early_ = true;
}

template <uint16_t N, typename T>
inline T RegisterContext::GetArg() const {
  static_assert(sizeof(T) <= sizeof(reg_t), "Invalid type");
  static_assert(N <= 7 || !(std::is_same_v<T, float> || std::is_same_v<T, double>),
                "Unsupported argument index");

  if constexpr (N == 0) return get_reg<T>(a0, fa0);
  else if constexpr (N == 1) return get_reg<T>(a1, fa1);
  else if constexpr (N == 2) return get_reg<T>(a2, fa2);
  else if constexpr (N == 3) return get_reg<T>(a3, fa3);
  else if constexpr (N == 4) return get_reg<T>(a4, fa4);
  else if constexpr (N == 5) return get_reg<T>(a5, fa5);
  else if constexpr (N == 6) return get_reg<T>(a6, fa6);
  else if constexpr (N == 7) return get_reg<T>(a7, fa7);
  else return force_cast<T>(reinterpret_cast<reg_t*>(sp)[N - 8]);
}

template <uint16_t N, typename T>
inline void RegisterContext::SetArg(T value) {
  static_assert(sizeof(T) <= sizeof(reg_t), "Invalid type");
  static_assert(N <= 7 || !(std::is_same_v<T, float> || std::is_same_v<T, double>),
                "Unsupported argument index");

  if constexpr (N == 0) set_reg(a0, fa0, value);
  else if constexpr (N == 1) set_reg(a1, fa1, value);
  else if constexpr (N == 2) set_reg(a2, fa2, value);
  else if constexpr (N == 3) set_reg(a3, fa3, value);
  else if constexpr (N == 4) set_reg(a4, fa4, value);
  else if constexpr (N == 5) set_reg(a5, fa5, value);
  else if constexpr (N == 6) set_reg(a6, fa6, value);
  else if constexpr (N == 7) set_reg(a7, fa7, value);
  else reinterpret_cast<reg_t*>(sp)[N - 8] = force_cast<reg_t>(value);
}

template <typename T, typename V>
constexpr inline T RegisterContext::force_cast(V value) {
  if constexpr (std::is_same_v<T, V>) return value;
  else return (T)value;
}

template <typename T>
constexpr inline T RegisterContext::get_reg(reg_t r, freg_t fr) {
  if constexpr (std::is_same_v<T, float>) return fr.f;
  else if constexpr (std::is_same_v<T, double>) return fr.d;
  else return force_cast<T>(r);
}

template <typename T>
constexpr inline void RegisterContext::set_reg(reg_t& r, freg_t& fr, T v) {
  if constexpr (std::is_same_v<T, float>) fr.f = v;
  else if constexpr (std::is_same_v<T, double>) fr.d = v;
  else r = force_cast<reg_t>(v);
}

inline TrampolineAllocator::TrampolineAllocator(TrampolineType t)
    : type(t), custom_alloc(nullptr), custom_free(nullptr), data(nullptr) {
}

inline TrampolineAllocator::TrampolineAllocator(decltype(custom_alloc) a,
                                                decltype(custom_free) f,
                                                void* d)
    : type(TrampolineType::kCustom), custom_alloc(a), custom_free(f), data(d) {
}

template <typename Func>
inline ScopedRWXMemory::ScopedRWXMemory(Func address)
    : ScopedRWXMemory(reinterpret_cast<func_t>(address), kRead | kExec) {
}

inline bool ScopedRWXMemory::IsValid() {
  return address_ != nullptr;
}

}  // namespace rv64hook

#else

typedef struct _RV64_HookHandle {
  void* address;
  void* backup;
} RV64_HookHandle;

typedef struct _RV64_RegisterContext {
  unsigned long ra, sp, gp, tp, t0, t1, t2, s0, fp, s1, a0, a1, a2, a3, a4, a5, a6, a7, s2, s3, s4,
      s5, s6, s7, s8, s9, s10, s11, t3, t4, t5, t6;

  union {
    float f;
    double d;
    unsigned long raw;
  } ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7, fs0, fs1, fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7, fs2,
      fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11, ft8, ft9, ft10, ft11;

  bool return_early;
} RV64_RegisterContext;

// clang-format off
RV64_HookHandle* RV64_InlineHook(void* address, void* hook, void** backup, uint32_t flags) __asm__("_ZN8rv64hook10InlineHookEPvS0_PS0_j");

RV64_HookHandle* RV64_InlineInstrument(
    void* address,
    void (*pre_handler)(RV64_RegisterContext*, RV64_HookHandle*, void*),
    void (*post_handler)(RV64_RegisterContext*, RV64_HookHandle*, void*),
    void* data,
    void** backup,
    uint32_t flags) __asm__("_ZN8rv64hook16InlineInstrumentEPvPFvPNS_15RegisterContextEPNS_10HookHandleES0_ES6_S0_PS0_j");

bool RV64_WriteTrampoline(void* address, void* hook, void** backup) __asm__("_ZN8rv64hook15WriteTrampolineEPvS0_PS0_");

bool RV64_SetEnabled(RV64_HookHandle* handle, bool enabled) __asm__("_ZN8rv64hook10HookHandle10SetEnabledEb");

void RV64_Unhook(RV64_HookHandle* handle) __asm__("_ZN8rv64hook10HookHandle6UnhookEv");

void RV64_UnhookAll(RV64_HookHandle* handle) __asm__("_ZN8rv64hook10HookHandle9UnhookAllEv");

bool RV64_InlineUnhook(void* address) __asm__("_ZN8rv64hook12InlineUnhookEPv");

const char* RV64_GetLastError() __asm__("_ZN8rv64hook12GetLastErrorEv");
// clang-format on

#endif

#if !defined(__riscv) || __riscv_xlen != 64
#error unsupported
#endif
