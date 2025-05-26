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
#include <version>
#if __cplusplus >= 201907L  // three_way_comparison
#include <compare>
#endif
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#ifdef __cplusplus

namespace rv64hook {

typedef void* func_t;
typedef unsigned long reg_t;
typedef long sreg_t;
typedef __uint128_t reg128_t;

class HookHandle {
 public:
  [[nodiscard]] inline func_t GetAddress() const;

  [[nodiscard]] inline func_t GetBackup() const;

  bool SetEnabled(bool enabled);

  bool SetEnabledAll(bool enabled);

  bool Unhook();

  bool UnhookAll();

 protected:
  func_t address_;
  func_t backup_;
};

union freg_t {
  float f;
  double d;
  reg_t raw;

#ifdef __aarch64__
  long double q;
  reg128_t raw128;
  struct {
    float f0, f1, f2, f3;
  };
  struct {
    double d0, d1;
  };
#else
  struct {
    float f0, f1;
  };
#endif

  [[nodiscard]] constexpr inline operator float() const;

  [[nodiscard]] constexpr inline operator double() const;

  [[nodiscard]] constexpr inline operator reg_t() const;

  constexpr inline freg_t& operator=(const freg_t& x);

  constexpr inline freg_t& operator=(float x);

  constexpr inline freg_t& operator=(double x);

  constexpr inline freg_t& operator=(reg_t x);

#if __cplusplus >= 201907L  // three_way_comparison
  template <typename T>
  [[nodiscard]] constexpr inline std::strong_ordering operator<=>(T x) const noexcept;
#endif
};

class RegisterContext {
 public:
#ifdef __aarch64__
  reg_t x0, x1, x2, x3, x4, x5, x6, x7;
  reg_t x8, x9, x10, x11, x12, x13, x14, x15;
  reg_t x16, x17, x18, x19, x20, x21, x22, x23;
  reg_t x24, x25, x26, x27, x28;
  union {
    struct {
      reg_t fp, lr, sp;
    };
    struct {
      reg_t x29, x30, x31;
    };
  };

  freg_t q0, q1, q2, q3, q4, q5, q6, q7;
  freg_t q8, q9, q10, q11, q12, q13, q14, q15;
  freg_t q16, q17, q18, q19, q20, q21, q22, q23;
  freg_t q24, q25, q26, q27, q28, q29, q30, q31;
#else
  union {
    struct {
      reg_t ra, sp, gp, tp;
      reg_t t0, t1, t2;
      union {
        reg_t s0;
        reg_t fp;
      };
      reg_t s1;
      reg_t a0, a1, a2, a3, a4, a5, a6, a7;
      reg_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
      reg_t t3, t4, t5, t6;

      freg_t ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7;
      freg_t fs0, fs1;
      freg_t fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7;
      freg_t fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11;
      freg_t ft8, ft9, ft10, ft11;
    };
    struct {
      reg_t x1, x2, x3, x4, x5, x6, x7, x8;
      reg_t x9, x10, x11, x12, x13, x14, x15, x16;
      reg_t x17, x18, x19, x20, x21, x22, x23, x24;
      reg_t x25, x26, x27, x28, x29, x30, x31;

      freg_t f0, f1, f2, f3, f4, f5, f6, f7;
      freg_t f8, f9, f10, f11, f12, f13, f14, f15;
      freg_t f16, f17, f18, f19, f20, f21, f22, f23;
      freg_t f24, f25, f26, f27, f28, f29, f30, f31;
    } regs;
  };
#endif

  template <typename T = reg_t>
  [[nodiscard]] inline T GetReturnValue() const;

  template <typename T>
  inline void ReturnValue(T value);

  inline void ReturnVoid();

  [[nodiscard]] inline bool IsReturned() const;

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
#ifdef __aarch64__
  // 占用被hook函数头 16 字节, 跳板分配在 ∞
  kWide = 1,
  // 占用被hook函数头 4 字节, 跳板分配在 PC +- 0~0xF'FFFF
  kPC26 = 2,
  // 占用被hook函数头 12 字节, 跳板分配在 0~0xFFFF'FFFF
  kVA32 = 3,
  kDefault = kVA32,
#else
  // 占用被hook函数头 18~20 字节, 跳板分配在 ∞
  kWide = 1,
  // 占用被hook函数头 4 字节, 跳板分配在 PC +- 0~0xF'FFFF
  kPC20 = 2,
  // 占用被hook函数头 8 字节, 跳板分配在 PC + 0~0xFFFF'FFFF
  kPC32 = 3,
  // 占用被hook函数头 8 字节, 跳板分配在 0~0xFFFF'FFFF
  kVA32 = 4,
  kDefault = kPC32,
#endif
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

HookHandle* InlineHook(func_t address, func_t hook, func_t* backup = nullptr);

[[nodiscard]] bool IsHookable(func_t func, size_t func_size);

HookHandle* InlineInstrument(func_t address,
                             RegisterHandler pre_handler,
                             RegisterHandler post_handler,
                             void* data = nullptr,
                             func_t* backup = nullptr);

int WriteTrampoline(func_t address, func_t hook, func_t* backup = nullptr);

bool InlineUnhook(func_t address);

bool SetTrampolineAllocator(TrampolineAllocator allocator);

[[nodiscard]] const char* GetLastError();

// ========================= Templates =========================

template <typename Func, typename MayLambda = Func>
static inline auto InlineHook(Func address, MayLambda hook, Func* backup = nullptr) {
  return InlineHook(reinterpret_cast<func_t>(address),
                    reinterpret_cast<func_t>(static_cast<Func>(hook)),
                    reinterpret_cast<func_t*>(backup));
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
                                    Func* backup = nullptr) {
  return InlineInstrument(reinterpret_cast<func_t>(address),
                          reinterpret_cast<RegisterHandler>(callbacks.pre),
                          reinterpret_cast<RegisterHandler>(callbacks.post),
                          static_cast<void*>(data),
                          reinterpret_cast<func_t*>(backup));
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

class Args {
 public:
  inline Args(RegisterContext* ctx);

  template <typename T = reg_t>
  inline T* next();

  template <typename T = reg_t>
  inline T* next() const;

  template <typename T = reg_t>
  [[nodiscard]] inline T get();

  template <typename T = reg_t>
  [[nodiscard]] inline T get() const;

  template <typename T = reg_t>
  inline void set(T value);

  template <typename T = reg_t>
  inline void set(T value) const;

  template <typename T = reg_t>
  inline void skip(uint32_t n);

 private:
  template <typename T, bool kConst>
  inline T* next() const;

  template <typename T, bool kConst>
  inline T get() const;

  template <typename T, bool kConst>
  inline void set(T value) const;

  RegisterContext* ctx_;
  mutable reg_t sp_;
  mutable uint32_t x_;
  mutable uint32_t f_;
  mutable reg_t memory0_;
  mutable reg_t memory1_;
};

// ==================================================

inline func_t HookHandle::GetAddress() const {
  return address_;
}

inline func_t HookHandle::GetBackup() const {
  return backup_;
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

#if __cplusplus >= 201907L  // three_way_comparison
template <typename T>
constexpr inline std::strong_ordering freg_t::operator<=>(T x) const noexcept {
  if constexpr (std::is_same_v<T, float>) {
    return f <=> x;
  } else if constexpr (std::is_same_v<T, double>) {
    return d <=> x;
#ifdef __aarch64__
  } else if constexpr (std::is_same_v<T, long double>) {
    return q <=> x;
  } else if constexpr (std::is_same_v<T, reg128_t>) {
    return raw128 <=> x;
#endif
  } else {
    static_assert(std::is_integral_v<T>, "Unsupported type");
    return static_cast<T>(raw) <=> x;
  }
}
#else
constexpr inline bool operator==(const freg_t& a, const freg_t& b) noexcept {
  return a.raw == b.raw;
}

constexpr inline bool operator==(const freg_t& a, float b) noexcept {
  return a.f == b;
}

constexpr inline bool operator==(const freg_t& a, double b) noexcept {
  return a.d == b;
}

constexpr inline bool operator==(const freg_t& a, reg_t b) {
  return a.raw == b;
}

#ifdef __aarch64__
constexpr inline bool operator==(const freg_t& a, long double b) noexcept {
  return a.q == b;
}

constexpr inline bool operator==(const freg_t& a, reg128_t b) noexcept {
  return a.raw128 == b;
}
#endif
#endif

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

inline bool RegisterContext::IsReturned() const {
  return return_early_;
}

#ifdef __aarch64__
template <uint16_t N, typename T>
inline T RegisterContext::GetArg() const {
  static_assert(sizeof(T) <= sizeof(reg_t), "Invalid type");
  static_assert(N <= 7 || !(std::is_same_v<T, float> || std::is_same_v<T, double>),
                "Unsupported argument index");

  if constexpr (N == 0) return get_reg<T>(x0, q0);
  else if constexpr (N == 1) return get_reg<T>(x1, q1);
  else if constexpr (N == 2) return get_reg<T>(x2, q2);
  else if constexpr (N == 3) return get_reg<T>(x3, q3);
  else if constexpr (N == 4) return get_reg<T>(x4, q4);
  else if constexpr (N == 5) return get_reg<T>(x5, q5);
  else if constexpr (N == 6) return get_reg<T>(x6, q6);
  else if constexpr (N == 7) return get_reg<T>(x7, q7);
  else return force_cast<T>(reinterpret_cast<reg_t*>(sp)[N - 8]);
}

template <uint16_t N, typename T>
inline void RegisterContext::SetArg(T value) {
  static_assert(sizeof(T) <= sizeof(reg_t), "Invalid type");
  static_assert(N <= 7 || !(std::is_same_v<T, float> || std::is_same_v<T, double>),
                "Unsupported argument index");

  if constexpr (N == 0) set_reg(x0, q0, value);
  else if constexpr (N == 1) set_reg(x1, q1, value);
  else if constexpr (N == 2) set_reg(x2, q2, value);
  else if constexpr (N == 3) set_reg(x3, q3, value);
  else if constexpr (N == 4) set_reg(x4, q4, value);
  else if constexpr (N == 5) set_reg(x5, q5, value);
  else if constexpr (N == 6) set_reg(x6, q6, value);
  else if constexpr (N == 7) set_reg(x7, q7, value);
  else reinterpret_cast<reg_t*>(sp)[N - 8] = force_cast<reg_t>(value);
}
#else
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
#endif

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

#ifdef __riscv
inline Args::Args(RegisterContext* ctx) : ctx_(ctx), sp_(ctx->sp), x_(0), f_(0) {
}

template <typename T>
inline T* Args::next() {
  return next<T, false>();
}

template <typename T>
inline T* Args::next() const {
  return next<T, true>();
}

template <typename T>
[[nodiscard]] inline T Args::get() {
  return get<T, false>();
}

template <typename T>
[[nodiscard]] inline T Args::get() const {
  return get<T, true>();
}

template <typename T>
inline void Args::set(T value) {
  set<T, false>(value);
}

template <typename T>
inline void Args::set(T value) const {
  set<T, true>(value);
}

template <typename T, bool kConst>
inline T* Args::next() const {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
  static_assert(std::is_reference_v<T>, "Use pointer instead of reference");
  static_assert(std::is_class_v<T>, "Use pointer instead of class");
  static_assert(std::is_union_v<T>, "Use pointer instead of union");
  static_assert(!(std::is_integral_v<T> && sizeof(T) <= sizeof(void*)) && !std::is_pointer_v<T> &&
                    !std::is_same_v<T, float> && !std::is_same_v<T, double> &&
                    !std::is_same_v<T, long double> && !std::is_same_v<T, __uint128_t>,
                "Unsupported type");

  if constexpr ((std::is_integral_v<T> && sizeof(T) <= sizeof(void*)) || std::is_pointer_v<T>) {
    if (x_ <= 7) [[likely]] {
      auto x = kConst ? x_ : x_++;
      return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ctx_) +
                                  offsetof(RegisterContext, a0) + (x * sizeof(void*)));
    } else {
      auto sp = sp_;
      if constexpr (!kConst) {
        sp_ += sizeof(void*);
      }
      return reinterpret_cast<T*>(sp);
    }
  } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
    if (f_ <= 7) [[likely]] {
      auto f = kConst ? f_ : f_++;
      return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ctx_) +
                                  offsetof(RegisterContext, fa0) + (f * sizeof(void*)));
    } else if (x_ <= 7) {
      auto x = kConst ? x_ : x_++;
      return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ctx_) +
                                  offsetof(RegisterContext, a0) + (x * sizeof(void*)));
    } else {
      auto sp = sp_;
      if constexpr (!kConst) {
        sp_ += sizeof(void*);
      }
      return reinterpret_cast<T*>(sp);
    }
  } else if constexpr (std::is_same_v<T, long double> || std::is_same_v<T, __uint128_t>) {
    if (x_ <= 7) [[likely]] {
      auto x = kConst ? x_ : x_++;
      if (x < 7) {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ctx_) +
                                    offsetof(RegisterContext, a0) + (x * sizeof(void*)));
      } else {
        memory0_ = ctx_->a7;
        memory1_ = *reinterpret_cast<reg_t*>(sp_);
        if constexpr (!kConst) {
          sp_ += sizeof(void*);
        }
        return reinterpret_cast<T*>(&memory0_);
      }
    } else {
      auto sp = kConst ? sp_ : sp_ += sizeof(T);
      return reinterpret_cast<T*>(sp);
    }
  } else {
    __builtin_unreachable();
  }
#pragma clang diagnostic pop
}

template <typename T, bool kConst>
inline T Args::get() const {
  return *next<T, kConst>();
}

template <typename T, bool kConst>
inline void Args::set(T value) const {
  auto p = next<T, kConst>();
  if constexpr (std::is_same_v<T, long double> || std::is_same_v<T, __uint128_t>) {
    if (p == reinterpret_cast<T*>(&memory0_)) {
      auto v = reinterpret_cast<reg_t*>(&value);
      ctx_->a7 = v[0];
      *reinterpret_cast<reg_t*>(kConst ? sp_ : sp_ - sizeof(void*)) = v[1];
      return;
    }
  }
  *p = value;
}

template <typename T>
inline void Args::skip(uint32_t n) {
  while (n--)
    next<T>();
}
#endif

}  // namespace rv64hook

#else

// ========================= C =========================

typedef struct _RV64_HookHandle {
  void* address;
  void* backup;
} RV64_HookHandle;

#ifdef __aarch64__
typedef struct _RV64_RegisterContext {
  unsigned long x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18,
      x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, fp, lr, sp;

  union {
    float f;
    double d;
    unsigned long raw;
    long double q;
    __uint128_t raw_128;
  } q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15, q16, q17, q18, q19, q20,
      q21, q22, q23, q24, q25, q26, q27, q28, q29, q30, q31;

  bool return_early;
} RV64_RegisterContext;

#else

typedef struct _RV64_RegisterContext {
  unsigned long ra, sp, gp, tp, t0, t1, t2, s0, s1, a0, a1, a2, a3, a4, a5, a6, a7, s2, s3, s4, s5,
      s6, s7, s8, s9, s10, s11, t3, t4, t5, t6;

  union {
    float f;
    double d;
    unsigned long raw;
  } ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7, fs0, fs1, fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7, fs2,
      fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11, ft8, ft9, ft10, ft11;

  bool return_early;
} RV64_RegisterContext;
#endif

// clang-format off
RV64_HookHandle* RV64_InlineHook(void* address, void* hook, void** backup) __asm__("_ZN8rv64hook10InlineHookEPvS0_PS0_j");

RV64_HookHandle* RV64_InlineInstrument(
    void* address,
    void (*pre_handler)(RV64_RegisterContext*, RV64_HookHandle*, void*),
    void (*post_handler)(RV64_RegisterContext*, RV64_HookHandle*, void*),
    void* data,
    void** backup) __asm__("_ZN8rv64hook16InlineInstrumentEPvPFvPNS_15RegisterContextEPNS_10HookHandleES0_ES6_S0_PS0_j");

bool RV64_WriteTrampoline(void* address, void* hook, void** backup) __asm__("_ZN8rv64hook15WriteTrampolineEPvS0_PS0_j");

bool RV64_SetEnabled(RV64_HookHandle* handle, bool enabled) __asm__("_ZN8rv64hook10HookHandle10SetEnabledEb");

bool RV64_SetEnabledAll(RV64_HookHandle* handle, bool enabled) __asm__("_ZN8rv64hook10HookHandle13SetEnabledAllEb");

void RV64_Unhook(RV64_HookHandle* handle) __asm__("_ZN8rv64hook10HookHandle6UnhookEv");

void RV64_UnhookAll(RV64_HookHandle* handle) __asm__("_ZN8rv64hook10HookHandle9UnhookAllEv");

bool RV64_InlineUnhook(void* address) __asm__("_ZN8rv64hook12InlineUnhookEPv");

const char* RV64_GetLastError() __asm__("_ZN8rv64hook12GetLastErrorEv");
// clang-format on

#endif

#if !(defined(__riscv) && __riscv_xlen == 64) && !defined(__aarch64__)
#error unsupported
#endif
