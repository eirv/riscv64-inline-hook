/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BERBERIS_ASSEMBLER_COMMON_RISCV_H_
#define BERBERIS_ASSEMBLER_COMMON_RISCV_H_

#include <cstddef>  // std::size_t
#include <cstdint>
#include <type_traits>  // std::enable_if, std::is_integral

#include "berberis/assembler/common.h"
#include "berberis/base/bit_util.h"
#include "berberis/base/checks.h"

namespace berberis {

namespace rv32e {

class Assembler;

}  // namespace rv32e

namespace rv32i {

class Assembler;

}  // namespace rv32i

namespace rv64i {

class Assembler;

}  // namespace rv64i

// riscv::Assembler includes implementation of most Risc V assembler instructions.
//
// RV32 and RV64 assemblers are nearly identical, but difference lies in handling
// of some instructions: RV32 uses certain encodings differently to handle compressed
// instructions, while RV64 adds some extra instructions to handle 32bit quantities
// (*not* 64bit quantities as the name implies, instead there are width-native instructions
// and extra 32bit ones for RV64).
//
// To handle that difference efficiently riscv::Assembler is CRTP class: it's parameterized
// by its own descendant and pull certain functions from its implementation.

namespace riscv {

template <typename DerivedAssemblerType>
class Assembler;

enum class Condition {
  kInvalidCondition = -1,

  kEqual = 0,
  kNotEqual = 1,
  kLess = 4,
  kGreaterEqual = 5,
  kBelow = 6,
  kAboveEqual = 7,
  kAlways = 8,
  kNever = 9,

  // aka...
  kCarry = kBelow,
  kNotCarry = kAboveEqual,
  kZero = kEqual,
  kNotZero = kNotEqual
};

enum class Csr {
  kFFlags = 0b00'00'0000'0001,
  kFrm = 0b00'00'0000'0010,
  kFCsr = 0b00'00'0000'0011,
  kVstart = 0b00'00'0000'1000,
  kVxsat = 0b00'00'0000'1001,
  kVxrm = 0b00'00'0000'1010,
  kVcsr = 0b00'00'0000'1111,
  kCycle = 0b11'00'0000'0000,
  kVl = 0b11'00'0010'0000,
  kVtype = 0b11'00'0010'0001,
  kVlenb = 0b11'00'0010'0010,
};

enum class Rounding { kRne = 0, kRtz = 1, kRdn = 2, kRup = 3, kRmm = 4, kDyn = 7 };

// Immediates are kept in a form ready to be used with emitter.
class BImmediate;
class CsrImmediate;
class IImmediate;
using Immediate = IImmediate;
class JImmediate;
// In RISC V manual shifts are described as using I-format with complex restrictions for which
// immediates are accepted and allowed (with parts of what manual classifies as “immediate” used
// to determine the actual instruction used and rules which differ between RV32 and RV64!).
//
// Instead of doing special handling for the instructions in python scripts we just reclassify
// these parts of immediate as “opcode” and reclassify these instructions as “Shift32-type” and
// “Shift64-type”.
//
// This also means that the same instructions for RV32 and RV64 would have different types, but
// since we don't have a goal to make RV32 a strict subset of RV64 that's acceptable.
//
// In addition we provide aliases in RV32 and RV64 assemblers to make sure users of assembler may
// still use ShiftImmediate and MakeShiftImmediate for native width without thinking about
// details of implementation.
class Shift32Immediate;
class Shift64Immediate;
class PImmediate;
class SImmediate;
class UImmediate;

// Don't use templates here to enable implicit conversions.
#define BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate)    \
  constexpr std::optional<Immediate> MakeImmediate(int8_t value);   \
  constexpr std::optional<Immediate> MakeImmediate(uint8_t value);  \
  constexpr std::optional<Immediate> MakeImmediate(int16_t value);  \
  constexpr std::optional<Immediate> MakeImmediate(uint16_t value); \
  constexpr std::optional<Immediate> MakeImmediate(int32_t value);  \
  constexpr std::optional<Immediate> MakeImmediate(uint32_t value); \
  constexpr std::optional<Immediate> MakeImmediate(int64_t value);  \
  constexpr std::optional<Immediate> MakeImmediate(uint64_t value)
BERBERIS_DEFINE_MAKE_IMMEDIATE(BImmediate, MakeBImmediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(CsrImmediate, MakeCsrImmediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(IImmediate, MakeImmediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(IImmediate, MakeIImmediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(JImmediate, MakeJImmediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(PImmediate, MakePImmediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(Shift32Immediate, MakeShift32Immediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(Shift64Immediate, MakeShift64Immediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(SImmediate, MakeSImmediate);
BERBERIS_DEFINE_MAKE_IMMEDIATE(UImmediate, MakeUImmediate);
#undef BERBERIS_DEFINE_MAKE_IMMEDIATE

// RawImmediate is used to bypass checks in constructor. It's not supposed to be used directly.
class RawImmediate {
 private:
  friend class BImmediate;
  friend class CsrImmediate;
  friend class IImmediate;
  friend class JImmediate;
  friend class Shift32Immediate;
  friend class Shift64Immediate;
  friend class PImmediate;
  friend class SImmediate;
  friend class UImmediate;
  template <typename DerivedAssemblerType>
  friend class Assembler;

  constexpr RawImmediate(int32_t value) : value_(value) {}
  int32_t value_;
};

#define BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, IntType)  \
  constexpr Immediate(IntType value) : Immediate(MakeRaw(value)) { \
    CHECK(AccetableValue(value));                                  \
  }
#define BERBERIS_DEFINE_IMMEDIATE(Immediate, MakeImmediate, kMaskValue, ...)                     \
  class Immediate {                                                                              \
   public:                                                                                       \
    static constexpr int32_t kMask = static_cast<int32_t>(kMaskValue);                           \
                                                                                                 \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, int8_t)                                     \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, uint8_t)                                    \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, int16_t)                                    \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, uint16_t)                                   \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, int32_t)                                    \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, uint32_t)                                   \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, int64_t)                                    \
    BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR(Immediate, uint64_t)                                   \
                                                                                                 \
    constexpr Immediate() : value_(0) {}                                                         \
                                                                                                 \
    constexpr int32_t EncodedValue() {                                                           \
      return value_;                                                                             \
    }                                                                                            \
                                                                                                 \
    friend bool operator==(Immediate const&, Immediate const&) = default;                        \
                                                                                                 \
    template <typename DerivedAssemblerType>                                                     \
    friend class Assembler;                                                                      \
    friend constexpr std::optional<Immediate> MakeImmediate(int8_t value);                       \
    friend constexpr std::optional<Immediate> MakeImmediate(uint8_t value);                      \
    friend constexpr std::optional<Immediate> MakeImmediate(int16_t value);                      \
    friend constexpr std::optional<Immediate> MakeImmediate(uint16_t value);                     \
    friend constexpr std::optional<Immediate> MakeImmediate(int32_t value);                      \
    friend constexpr std::optional<Immediate> MakeImmediate(uint32_t value);                     \
    friend constexpr std::optional<Immediate> MakeImmediate(int64_t value);                      \
    friend constexpr std::optional<Immediate> MakeImmediate(uint64_t value);                     \
    __VA_ARGS__                                                                                  \
                                                                                                 \
   private:                                                                                      \
    constexpr Immediate(RawImmediate raw) : value_(raw.value_) {}                                \
    /* Return true if value would fit into immediate. */                                         \
    template <typename IntType>                                                                  \
    static constexpr bool AccetableValue(IntType value);                                         \
    /* Make RawImmediate from immediate value. */                                                \
    /* Note: value is not checked for correctness! Public interface is MakeImmediate factory. */ \
    template <typename IntType>                                                                  \
    static constexpr RawImmediate MakeRaw(IntType value);                                        \
                                                                                                 \
    int32_t value_;                                                                              \
  }
BERBERIS_DEFINE_IMMEDIATE(
    BImmediate,
    MakeBImmediate,
    0xfe00'0f80,
    explicit constexpr operator int16_t() const {
      return ((value_ >> 7) & 0x001e) | ((value_ >> 20) & 0xf7e0) |
             ((value_ << 4) & 0x0800);
    }
    explicit constexpr operator int32_t() const {
      return ((value_ >> 7) & 0x0000'001e) | ((value_ >> 20) & 0xffff'f7e0) |
             ((value_ << 4) & 0x0000'0800);
    }
    explicit constexpr operator int64_t() const {
      return ((value_ >> 7) & 0x0000'0000'0000'001e) | ((value_ >> 20) & 0xffff'ffff'ffff'f7e0) |
             ((value_ << 4) & 0x0000'0000'0000'0800);
    });
BERBERIS_DEFINE_IMMEDIATE(
    CsrImmediate,
    MakeCsrImmediate,
    0x000f'8000,
    explicit constexpr operator int8_t() const { return value_ >> 15; }
    explicit constexpr operator uint8_t() const { return value_ >> 15; }
    explicit constexpr operator int16_t() const { return value_ >> 15; }
    explicit constexpr operator uint16_t() const { return value_ >> 15; }
    explicit constexpr operator int32_t() const { return value_ >> 15; }
    explicit constexpr operator uint32_t() const { return value_ >> 15; }
    explicit constexpr operator int64_t() const { return value_ >> 15;}
    explicit constexpr operator uint64_t() const { return value_ >> 15; });
BERBERIS_DEFINE_IMMEDIATE(
    IImmediate, MakeIImmediate, 0xfff0'0000, constexpr IImmediate(SImmediate s_imm);

    explicit constexpr operator int16_t() const { return value_ >> 20; }
    explicit constexpr operator int32_t() const { return value_ >> 20; }
    explicit constexpr operator int64_t() const { return value_ >> 20; }

    friend SImmediate;);
BERBERIS_DEFINE_IMMEDIATE(
    JImmediate,
    MakeJImmediate,
    0xffff'f000,
    explicit constexpr operator int32_t() const {
      return ((value_ >> 20) & 0xfff0'07fe) | ((value_ >> 9) & 0x0000'0800) |
             (value_ & 0x000f'f000);
    }
    explicit constexpr operator int64_t() const {
      return ((value_ >> 20) & 0xffff'ffff'fff0'07fe) | ((value_ >> 9) & 0x0000'0000'0000'0800) |
             (value_ & 0x0000'0000'000f'f000);
    });
BERBERIS_DEFINE_IMMEDIATE(
    PImmediate,
    MakePImmediate,
    0xfe00'0000,
    explicit constexpr
    operator int16_t() const { return value_ >> 20; }
    explicit constexpr operator int32_t() const { return value_ >> 20; }
    explicit constexpr operator int64_t() const { return value_ >> 20; });
BERBERIS_DEFINE_IMMEDIATE(
    Shift32Immediate,
    MakeShift32Immediate,
    0x01f0'0000,
    explicit constexpr operator int8_t() const { return value_ >> 20; }
    explicit constexpr operator uint8_t() const { return value_ >> 20; }
    explicit constexpr operator int16_t() const { return value_ >> 20; }
    explicit constexpr operator uint16_t() const { return value_ >> 20; }
    explicit constexpr operator int32_t() const { return value_ >> 20; }
    explicit constexpr operator uint32_t() const { return value_ >> 20; }
    explicit constexpr operator int64_t() const { return value_ >> 20;}
    explicit constexpr operator uint64_t() const { return value_ >> 20; });
BERBERIS_DEFINE_IMMEDIATE(
    Shift64Immediate,
    MakeShift64Immediate,
    0x03f0'0000,
    explicit constexpr operator int8_t() const { return value_ >> 20; }
    explicit constexpr operator uint8_t() const { return value_ >> 20; }
    explicit constexpr operator int16_t() const { return value_ >> 20; }
    explicit constexpr operator uint16_t() const { return value_ >> 20; }
    explicit constexpr operator int32_t() const { return value_ >> 20; }
    explicit constexpr operator uint32_t() const { return value_ >> 20; }
    explicit constexpr operator int64_t() const { return value_ >> 20;}
    explicit constexpr operator uint64_t() const { return value_ >> 20; });
BERBERIS_DEFINE_IMMEDIATE(
    SImmediate, MakeSImmediate, 0xfe00'0f80, constexpr SImmediate(Immediate imm);

    explicit constexpr operator int16_t() const {
      return ((value_ >> 7) & 0x0000'001f) | (value_ >> 20);
    }
    explicit constexpr operator int32_t() const {
      return ((value_ >> 7) & 0x0000'001f) | (value_ >> 20);
    }
    explicit constexpr operator int64_t() const {
      return ((value_ >> 7) & 0x0000'001f) | (value_ >> 20);
    }

    friend class IImmediate;);
BERBERIS_DEFINE_IMMEDIATE(
    UImmediate,
    MakeUImmediate,
    0xffff'f000,
    explicit constexpr operator int32_t() const { return value_; }
    explicit constexpr operator int64_t() const { return value_; });
#undef BERBERIS_DEFINE_IMMEDIATE
#undef BERBERIS_DEFINE_IMMEDIATE_CONSTRUCTOR

constexpr IImmediate::IImmediate(SImmediate s_imm)
    : value_((s_imm.value_ & 0xfe00'0000) | ((s_imm.value_ & 0x0000'0f80) << 13)) {}

constexpr SImmediate::SImmediate(Immediate imm)
    : value_((imm.value_ & 0xfe00'0000) | ((imm.value_ & 0x01f0'0000) >> 13)) {}

#define BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, IntType) \
  constexpr std::optional<Immediate> MakeImmediate(IntType value) {       \
    if (!Immediate::AccetableValue(value)) {                              \
      return std::nullopt;                                                \
    }                                                                     \
    return Immediate{Immediate::MakeRaw(value)};                          \
  }
#define BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(Immediate, MakeImmediate) \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, int8_t)   \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, uint8_t)  \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, int16_t)  \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, uint16_t) \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, int32_t)  \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, uint32_t) \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, int64_t)  \
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate, uint64_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(BImmediate, MakeBImmediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(CsrImmediate, MakeCsrImmediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(IImmediate, MakeIImmediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(JImmediate, MakeJImmediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(PImmediate, MakePImmediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(Shift32Immediate, MakeShift32Immediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(Shift64Immediate, MakeShift64Immediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(SImmediate, MakeSImmediate)
BERBERIS_DEFINE_MAKE_IMMEDIATE_SET(UImmediate, MakeUImmediate)
#undef BERBERIS_DEFINE_MAKE_IMMEDIATE_SET
#undef BERBERIS_DEFINE_MAKE_IMMEDIATE

#define BERBERIS_DEFINE_MAKE_IMMEDIATE(IntType)                     \
  constexpr std::optional<Immediate> MakeImmediate(IntType value) { \
    return MakeIImmediate(value);                                   \
  }
BERBERIS_DEFINE_MAKE_IMMEDIATE(int8_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE(uint8_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE(int16_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE(uint16_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE(int32_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE(uint32_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE(int64_t)
BERBERIS_DEFINE_MAKE_IMMEDIATE(uint64_t)
#undef BERBERIS_DEFINE_MAKE_IMMEDIATE

// Return true if value would fit into B-immediate.
template <typename IntType>
constexpr bool BImmediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // B-immediate accepts 12 bits, but encodes signed even values, that's why we only may accept
  // low 12 bits of any unsigned value.
  // Encode mask as the largest accepted value plus one and cut it to IntType size.
  constexpr uint64_t kUnsigned64bitInputMask = 0xffff'ffff'ffff'f001;
  if constexpr (!std::is_signed_v<IntType>) {
    constexpr IntType kUnsignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kUnsignedInputMask) == IntType{0};
  } else {
    // For signed values we also accept the same values as for unsigned case, but also accept
    // value that have all bits in am kUnsignedInputMask set.
    // B-immediate compresses these into one single sign bit, but lowest bit have to be zero.
    constexpr IntType kSignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kSignedInputMask) == IntType{0} ||
           static_cast<IntType>(value & kSignedInputMask) == (kSignedInputMask & ~int64_t{1});
  }
}

// Return true if value would fit into Csr-immediate.
template <typename IntType>
constexpr bool CsrImmediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Csr immediate is unsigned immediate with possible values between 0 and 31.
  // If we make value unsigned negative numbers would become numbers >127 and would be rejected.
  return std::make_unsigned_t<IntType>(value) < 32;
}

// Return true if value would fit into immediate.
template <typename IntType>
constexpr bool Immediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // I-immediate accepts 12 bits, but encodes signed values, that's why we only may accept low
  // 11 bits of any unsigned value.
  // Encode mask as the largest accepted value plus one and cut it to IntType size.
  constexpr uint64_t kUnsigned64bitInputMask = 0xffff'ffff'ffff'f800;
  if constexpr (!std::is_signed_v<IntType>) {
    constexpr IntType kUnsignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kUnsignedInputMask) == IntType{0};
  } else {
    // For signed values we accept the same values as for unsigned case, but also accept
    // values that have all bits in kUnsignedInputMask set.
    // I-immediate compresses these into one single sign bit.
    constexpr IntType kSignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kSignedInputMask) == IntType{0} ||
           static_cast<IntType>(value & kSignedInputMask) == kSignedInputMask;
  }
}

// Return true if value would fit into J-immediate.
template <typename IntType>
constexpr bool JImmediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // J-immediate accepts 20 bits, but encodes signed even values, that's why we only may accept
  // bits from 1 to 19 of any unsigned value. Encode mask as the largest accepted value plus 1 and
  // cut it to IntType size.
  constexpr uint64_t kUnsigned64bitInputMask = 0xffff'ffff'fff0'0001;
  if constexpr (!std::is_signed_v<IntType>) {
    constexpr IntType kUnsignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kUnsignedInputMask) == IntType{0};
  } else {
    // For signed values we accept the same values as for unsigned case, but also accept
    // value that have all bits in kUnsignedInputMask set except zero bit (which is zero).
    // J-immediate compresses these into one single sign bit, but lowest bit have to be zero.
    constexpr IntType kSignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kSignedInputMask) == IntType{0} ||
           static_cast<IntType>(value & kSignedInputMask) == (kSignedInputMask & ~int64_t{1});
  }
}

// Return true if value would fit into P-immediate.
template <typename IntType>
constexpr bool PImmediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // P-immediate accepts 7 bits, but encodes only values divisible by 32, that's why we only may
  // accept bits from 5 to 10 of any unsigned value. Encode mask as the largest accepted value
  // plus 31 and cut it to IntType size.
  constexpr uint64_t kUnsigned64bitInputMask = 0xffff'ffff'ffff'f81f;
  if constexpr (!std::is_signed_v<IntType>) {
    constexpr IntType kUnsignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kUnsignedInputMask) == IntType{0};
  } else {
    // For signed values we accept the same values as for unsigned case, but also accept
    // value that have all bits in kUnsignedInputMask set except the lowest 5 bits (which are
    // zero). P-immediate compresses these into one single sign bit, but lowest bits have to be
    // zero.
    constexpr IntType kSignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kSignedInputMask) == IntType{0} ||
           static_cast<IntType>(value & kSignedInputMask) == (kSignedInputMask & ~int64_t{0x1f});
  }
}

// Return true if value would fit into Shift32-immediate.
template <typename IntType>
constexpr bool Shift32Immediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Shift32 immediate is unsigned immediate with possible values between 0 and 31.
  // If we make value unsigned negative numbers would become numbers >127 and would be rejected.
  return std::make_unsigned_t<IntType>(value) < 32;
}

// Return true if value would fit into Shift64-immediate.
template <typename IntType>
constexpr bool Shift64Immediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Shift64 immediate is unsigned immediate with possible values between 0 and 63.
  // If we make value unsigned negative numbers would become numbers >127 and would be rejected.
  return std::make_unsigned_t<IntType>(value) < 64;
}

// Immediate (I-immediate in RISC V documentation) and S-Immediate are siblings: they encode
// the same values but in a different way.
// AccetableValue are the same for that reason, but MakeRaw are different.
template <typename IntType>
constexpr bool SImmediate::AccetableValue(IntType value) {
  return Immediate::AccetableValue(value);
}

// Return true if value would fit into U-immediate.
template <typename IntType>
constexpr bool UImmediate::AccetableValue(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // U-immediate accepts 20 bits, but encodes only values divisible by 4096, that's why we only
  // may accept bits from 12 to 30 of any unsigned value. Encode mask as the largest accepted
  // value plus 4095 and cut it to IntType size.
  constexpr uint64_t kUnsigned64bitInputMask = 0xffff'ffff'8000'0fff;
  if constexpr (!std::is_signed_v<IntType>) {
    constexpr IntType kUnsignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kUnsignedInputMask) == IntType{0};
  } else {
    // For signed values we accept the same values as for unsigned case, but also accept
    // value that have all bits in kUnsignedInputMask set except lower 12 bits (which are zero).
    // U-immediate compresses these into one single sign bit, but lowest bits have to be zero.
    constexpr IntType kSignedInputMask = static_cast<IntType>(kUnsigned64bitInputMask);
    return static_cast<IntType>(value & kSignedInputMask) == IntType{0} ||
           static_cast<IntType>(value & kSignedInputMask) == (kSignedInputMask & ~int64_t{0xfff});
  }
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeBImmediate factory.
template <typename IntType>
constexpr RawImmediate BImmediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Note: we have to convert type to int32_t before processing it! Otherwise we would produce
  // incorrect value for negative inputs since one single input sign in the small immediate would
  // turn into many bits in the insruction.
  return (static_cast<int32_t>(value) & static_cast<int32_t>(0x8000'0000)) |
         ((static_cast<int32_t>(value) & static_cast<int32_t>(0x0000'0800)) >> 4) |
         ((static_cast<int32_t>(value) & static_cast<int32_t>(0x0000'001f)) << 7) |
         ((static_cast<int32_t>(value) & static_cast<int32_t>(0x0000'07e0)) << 20);
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeImmediate factory.
template <typename IntType>
constexpr RawImmediate CsrImmediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Note: this is correct if input value is between 0 and 31, but that would be checked in
  // MakeCsrImmediate.
  return static_cast<int32_t>(value) << 15;
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeImmediate factory.
template <typename IntType>
constexpr RawImmediate Immediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  return static_cast<int32_t>(value) << 20;
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeJImmediate factory.
template <typename IntType>
constexpr RawImmediate JImmediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Note: we have to convert type to int32_t before processing it! Otherwise we would produce
  // incorrect value for negative inputs since one single input sign in the small immediate would
  // turn into many bits in the insruction.
  return (static_cast<int32_t>(value) & static_cast<int32_t>(0x800f'f000)) |
         ((static_cast<int32_t>(value) & static_cast<int32_t>(0x0000'0800)) << 9) |
         ((static_cast<int32_t>(value) & static_cast<int32_t>(0x0000'07fe)) << 20);
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeImmediate factory.
template <typename IntType>
constexpr RawImmediate PImmediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Note: this is correct if input value is divisible by 32, but that would be checked in
  // MakePImmediate.
  return static_cast<int32_t>(value) << 20;
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeImmediate factory.
template <typename IntType>
constexpr RawImmediate Shift32Immediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Note: this is correct if input value is between 0 and 31, but that would be checked in
  // MakeShift32Immediate.
  return static_cast<int32_t>(value) << 20;
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeImmediate factory.
template <typename IntType>
constexpr RawImmediate Shift64Immediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Note: this is only correct if input value is between 0 and 63, but that would be checked in
  // MakeShift64Immediate.
  return static_cast<int32_t>(value) << 20;
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeSImmediate factory.
template <typename IntType>
constexpr RawImmediate SImmediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Here, because we are only using platforms with 32bit ints conversion to 32bit signed int may
  // happen both before masking and after but we are doing it before for consistency.
  return ((static_cast<int32_t>(value) & static_cast<int32_t>(0xffff'ffe0)) << 20) |
         ((static_cast<int32_t>(value) & static_cast<int32_t>(0x0000'001f)) << 7);
}

// Make RawImmediate from immediate value.
// Note: value is not checked for correctness here! Public interface is MakeImmediate factory.
template <typename IntType>
constexpr RawImmediate UImmediate::MakeRaw(IntType value) {
  static_assert(std::is_integral_v<IntType>);
  static_assert(sizeof(IntType) <= sizeof(uint64_t));
  // Note: this is only correct if input value is between divisible by 4096 , but that would be
  // checked in MakeUImmediate.
  return static_cast<int32_t>(value);
}

template <typename DerivedAssemblerType>
class Assembler : public AssemblerBase {
 public:
  explicit Assembler(MachineCode* code) : AssemblerBase(code) {}

  using Condition = riscv::Condition;
  using Csr = riscv::Csr;
  using Rounding = riscv::Rounding;

  class Register {
   public:
    constexpr bool operator==(const Register& reg) const { return num_ == reg.num_; }
    constexpr bool operator!=(const Register& reg) const { return num_ != reg.num_; }
    constexpr uint8_t GetPhysicalIndex() { return num_; }
    friend constexpr uint8_t ValueForFmtSpec(Register value) { return value.num_; }
    friend class Assembler<DerivedAssemblerType>;
    friend class rv32e::Assembler;
    friend class rv32i::Assembler;
    friend class rv64i::Assembler;

   public:
    explicit constexpr Register(uint8_t num) : num_(num) {}
    uint8_t num_;
  };

  // Note: register x0, technically, can be specified in assembler even if it doesn't exist
  // as separate hardware register. It even have alias “zero” even in clang assembler.
  static constexpr Register x0{0};
  static constexpr Register x1{1};
  static constexpr Register x2{2};
  static constexpr Register x3{3};
  static constexpr Register x4{4};
  static constexpr Register x5{5};
  static constexpr Register x6{6};
  static constexpr Register x7{7};
  static constexpr Register x8{8};
  static constexpr Register x9{9};
  static constexpr Register x10{10};
  static constexpr Register x11{11};
  static constexpr Register x12{12};
  static constexpr Register x13{13};
  static constexpr Register x14{14};
  static constexpr Register x15{15};
  static constexpr Register x16{16};
  static constexpr Register x17{17};
  static constexpr Register x18{18};
  static constexpr Register x19{19};
  static constexpr Register x20{20};
  static constexpr Register x21{21};
  static constexpr Register x22{22};
  static constexpr Register x23{23};
  static constexpr Register x24{24};
  static constexpr Register x25{25};
  static constexpr Register x26{26};
  static constexpr Register x27{27};
  static constexpr Register x28{28};
  static constexpr Register x29{29};
  static constexpr Register x30{30};
  static constexpr Register x31{31};

  // Aliases
  static constexpr Register no_register{0x80};
  static constexpr Register zero{0};

  class FpRegister {
   public:
    constexpr bool operator==(const FpRegister& reg) const { return num_ == reg.num_; }
    constexpr bool operator!=(const FpRegister& reg) const { return num_ != reg.num_; }
    constexpr uint8_t GetPhysicalIndex() { return num_; }
    friend constexpr uint8_t ValueForFmtSpec(FpRegister value) { return value.num_; }
    friend class Assembler<DerivedAssemblerType>;

   private:
    explicit constexpr FpRegister(uint8_t num) : num_(num) {}
    uint8_t num_;
  };

  static constexpr FpRegister f0{0};
  static constexpr FpRegister f1{1};
  static constexpr FpRegister f2{2};
  static constexpr FpRegister f3{3};
  static constexpr FpRegister f4{4};
  static constexpr FpRegister f5{5};
  static constexpr FpRegister f6{6};
  static constexpr FpRegister f7{7};
  static constexpr FpRegister f8{8};
  static constexpr FpRegister f9{9};
  static constexpr FpRegister f10{10};
  static constexpr FpRegister f11{11};
  static constexpr FpRegister f12{12};
  static constexpr FpRegister f13{13};
  static constexpr FpRegister f14{14};
  static constexpr FpRegister f15{15};
  static constexpr FpRegister f16{16};
  static constexpr FpRegister f17{17};
  static constexpr FpRegister f18{18};
  static constexpr FpRegister f19{19};
  static constexpr FpRegister f20{20};
  static constexpr FpRegister f21{21};
  static constexpr FpRegister f22{22};
  static constexpr FpRegister f23{23};
  static constexpr FpRegister f24{24};
  static constexpr FpRegister f25{25};
  static constexpr FpRegister f26{26};
  static constexpr FpRegister f27{27};
  static constexpr FpRegister f28{28};
  static constexpr FpRegister f29{29};
  static constexpr FpRegister f30{30};
  static constexpr FpRegister f31{31};

  // ABI
  static constexpr FpRegister ft0{0};
  static constexpr FpRegister ft1{1};
  static constexpr FpRegister ft2{2};
  static constexpr FpRegister ft3{3};
  static constexpr FpRegister ft4{4};
  static constexpr FpRegister ft5{5};
  static constexpr FpRegister ft6{6};
  static constexpr FpRegister ft7{7};
  static constexpr FpRegister fs0{8};
  static constexpr FpRegister fs1{9};
  static constexpr FpRegister fa0{10};
  static constexpr FpRegister fa1{11};
  static constexpr FpRegister fa2{12};
  static constexpr FpRegister fa3{13};
  static constexpr FpRegister fa4{14};
  static constexpr FpRegister fa5{15};
  static constexpr FpRegister fa6{16};
  static constexpr FpRegister fa7{17};
  static constexpr FpRegister fs2{18};
  static constexpr FpRegister fs3{19};
  static constexpr FpRegister fs4{20};
  static constexpr FpRegister fs5{21};
  static constexpr FpRegister fs6{22};
  static constexpr FpRegister fs7{23};
  static constexpr FpRegister fs8{24};
  static constexpr FpRegister fs9{25};
  static constexpr FpRegister fs10{26};
  static constexpr FpRegister fs11{27};
  static constexpr FpRegister ft8{28};
  static constexpr FpRegister ft9{29};
  static constexpr FpRegister ft10{30};
  static constexpr FpRegister ft11{31};

  class VRegister {
   public:
    constexpr bool operator==(const VRegister& reg) const { return num_ == reg.num_; }
    constexpr bool operator!=(const VRegister& reg) const { return num_ != reg.num_; }
    constexpr uint8_t GetPhysicalIndex() { return num_; }
    friend constexpr uint8_t ValueForFmtSpec(VRegister value) { return value.num_; }
    friend class Assembler<DerivedAssemblerType>;

   private:
    explicit constexpr VRegister(uint8_t num) : num_(num) {}
    uint8_t num_;
  };

  static constexpr VRegister no_v_register{0x80};
  static constexpr VRegister v0{0};
  static constexpr VRegister v1{1};
  static constexpr VRegister v2{2};
  static constexpr VRegister v3{3};
  static constexpr VRegister v4{4};
  static constexpr VRegister v5{5};
  static constexpr VRegister v6{6};
  static constexpr VRegister v7{7};
  static constexpr VRegister v8{8};
  static constexpr VRegister v9{9};
  static constexpr VRegister v10{10};
  static constexpr VRegister v11{11};
  static constexpr VRegister v12{12};
  static constexpr VRegister v13{13};
  static constexpr VRegister v14{14};
  static constexpr VRegister v15{15};
  static constexpr VRegister v16{16};
  static constexpr VRegister v17{17};
  static constexpr VRegister v18{18};
  static constexpr VRegister v19{19};
  static constexpr VRegister v20{20};
  static constexpr VRegister v21{21};
  static constexpr VRegister v22{22};
  static constexpr VRegister v23{23};
  static constexpr VRegister v24{24};
  static constexpr VRegister v25{25};
  static constexpr VRegister v26{26};
  static constexpr VRegister v27{27};
  static constexpr VRegister v28{28};
  static constexpr VRegister v29{29};
  static constexpr VRegister v30{30};
  static constexpr VRegister v31{31};

  template <typename RegisterType, typename ImmediateType>
  struct Operand {
    RegisterType base{0};
    ImmediateType disp = 0;
  };

  using BImmediate = riscv::BImmediate;
  using CsrImmediate = riscv::CsrImmediate;
  using IImmediate = riscv::IImmediate;
  using Immediate = riscv::Immediate;
  using JImmediate = riscv::JImmediate;
  using Shift32Immediate = riscv::Shift32Immediate;
  using Shift64Immediate = riscv::Shift64Immediate;
  using PImmediate = riscv::PImmediate;
  using SImmediate = riscv::SImmediate;
  using UImmediate = riscv::UImmediate;

  // Don't use templates here to enable implicit conversions.
#define BERBERIS_DEFINE_MAKE_IMMEDIATE(Immediate, MakeImmediate)            \
  static constexpr std::optional<Immediate> MakeImmediate(int8_t value) {   \
    return riscv::MakeImmediate(value);                                     \
  }                                                                         \
  static constexpr std::optional<Immediate> MakeImmediate(uint8_t value) {  \
    return riscv::MakeImmediate(value);                                     \
  }                                                                         \
  static constexpr std::optional<Immediate> MakeImmediate(int16_t value) {  \
    return riscv::MakeImmediate(value);                                     \
  }                                                                         \
  static constexpr std::optional<Immediate> MakeImmediate(uint16_t value) { \
    return riscv::MakeImmediate(value);                                     \
  }                                                                         \
  static constexpr std::optional<Immediate> MakeImmediate(int32_t value) {  \
    return riscv::MakeImmediate(value);                                     \
  }                                                                         \
  static constexpr std::optional<Immediate> MakeImmediate(uint32_t value) { \
    return riscv::MakeImmediate(value);                                     \
  }                                                                         \
  static constexpr std::optional<Immediate> MakeImmediate(int64_t value) {  \
    return riscv::MakeImmediate(value);                                     \
  }                                                                         \
  static constexpr std::optional<Immediate> MakeImmediate(uint64_t value) { \
    return riscv::MakeImmediate(value);                                     \
  }
  BERBERIS_DEFINE_MAKE_IMMEDIATE(BImmediate, MakeBImmediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(CsrImmediate, MakeCsrImmediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(IImmediate, MakeImmediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(IImmediate, MakeIImmediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(JImmediate, MakeJImmediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(PImmediate, MakePImmediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Shift32Immediate, MakeShift32Immediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(Shift64Immediate, MakeShift64Immediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(SImmediate, MakeSImmediate)
  BERBERIS_DEFINE_MAKE_IMMEDIATE(UImmediate, MakeUImmediate)
#undef BERBERIS_DEFINE_MAKE_IMMEDIATE

  // Macro operations.
  void Finalize() { ResolveJumps(); }

  void ResolveJumps();

  // Instructions.
#include "berberis/assembler/gen_assembler_common_riscv-inl.h"  // NOLINT generated file!

 protected:
  // Information about operands.
  template <typename OperandType, typename = void>
  class OperandInfo;

  // Wrapped operand with information of where in the encoded instruction should it be placed.
  template <typename OperandMarker, typename RegisterType>
  struct RegisterOperand {
    constexpr int32_t EncodeImmediate() {
      return value.GetPhysicalIndex()
             << OperandInfo<RegisterOperand<OperandMarker, RegisterType>>::kOffset;
    }

    RegisterType value;
  };

  struct ConditionOperand {
    constexpr int32_t EncodeImmediate() {
      return static_cast<int32_t>(value) << OperandInfo<ConditionOperand>::kOffset;
    }

    Condition value;
  };

  struct RoundingOperand {
    constexpr int32_t EncodeImmediate() {
      return static_cast<int32_t>(value) << OperandInfo<RoundingOperand>::kOffset;
    }

    Rounding value;
  };

  // Operand class  markers. Note, these classes shouldn't ever be instantiated, they are just
  // used to carry information about operands.
  class RdMarker;
  class Rs1Marker;
  class Rs2Marker;
  class Rs3Marker;

  template <typename RegisterType>
  class OperandInfo<RegisterOperand<RdMarker, RegisterType>> {
   public:
    static constexpr bool IsImmediate = false;
    static constexpr uint8_t kOffset = 7;
    static constexpr uint32_t kMask = 0x0000'0f80;
  };

  template <>
  class OperandInfo<ConditionOperand> {
   public:
    static constexpr bool IsImmediate = false;
    static constexpr uint8_t kOffset = 12;
    static constexpr uint32_t kMask = 0x0000'7000;
  };

  template <>
  class OperandInfo<RoundingOperand> {
   public:
    static constexpr bool IsImmediate = false;
    static constexpr uint8_t kOffset = 12;
    static constexpr uint32_t kMask = 0x0000'7000;
  };

  template <typename RegisterType>
  class OperandInfo<RegisterOperand<Rs1Marker, RegisterType>> {
   public:
    static constexpr bool IsImmediate = false;
    static constexpr uint8_t kOffset = 15;
    static constexpr uint32_t kMask = 0x000f'8000;
  };

  template <typename RegisterType>
  class OperandInfo<RegisterOperand<Rs2Marker, RegisterType>> {
   public:
    static constexpr bool IsImmediate = false;
    static constexpr uint8_t kOffset = 20;
    static constexpr uint32_t kMask = 0x01f0'0000;
  };

  template <typename RegisterType>
  class OperandInfo<RegisterOperand<Rs3Marker, RegisterType>> {
   public:
    static constexpr bool IsImmediate = false;
    static constexpr uint8_t kOffset = 27;
    static constexpr uint32_t kMask = 0xf800'0000;
  };

  template <typename Immediate>
  class OperandInfo<Immediate, std::enable_if_t<sizeof(Immediate::kMask) != 0>> {
   public:
    static constexpr bool IsImmediate = true;
    static constexpr uint8_t kOffset = 0;
    static constexpr uint32_t kMask = Immediate::kMask;
  };

  template <typename RegisterType>
  RegisterOperand<RdMarker, RegisterType> Rd(RegisterType value) {
    return {value};
  }

  ConditionOperand Cond(Condition value) { return {value}; }

  RoundingOperand Rm(Rounding value) { return {value}; }

  template <typename RegisterType>
  RegisterOperand<Rs1Marker, RegisterType> Rs1(RegisterType value) {
    return {value};
  }

  template <typename RegisterType>
  RegisterOperand<Rs2Marker, RegisterType> Rs2(RegisterType value) {
    return {value};
  }

  template <typename RegisterType>
  RegisterOperand<Rs3Marker, RegisterType> Rs3(RegisterType value) {
    return {value};
  }

  template <uint32_t kOpcode, uint32_t kOpcodeMask, typename... ArgumentsTypes>
  void EmitInstruction(ArgumentsTypes... arguments) {
    // All uncompressed instructions in RISC-V have two lowest bit set and we don't handle
    // compressed instructions here.
    static_assert((kOpcode & 0b11) == 0b11);
    // Instruction shouldn't have any bits set outside of its opcode mask.
    static_assert((kOpcode & ~kOpcodeMask) == 0);
    // Places for all operands in the opcode should not intersect with opcode.
    static_assert((((kOpcodeMask & OperandInfo<ArgumentsTypes>::kMask) == 0) && ...));
    Emit32((kOpcode | ... | [](auto argument) {
      if constexpr (OperandInfo<decltype(argument)>::IsImmediate) {
        return argument.EncodedValue();
      } else {
        return argument.EncodeImmediate();
      }
    }(arguments)));
  }

  template <uint32_t kOpcode,
            typename ArgumentsType0,
            typename ArgumentsType1,
            typename ImmediateType>
  void EmitBTypeInstruction(ArgumentsType0&& argument0,
                            ArgumentsType1&& argument1,
                            ImmediateType&& immediate) {
    return EmitInstruction<kOpcode, 0x0000'707f>(Rs1(argument0), Rs2(argument1), immediate);
  }

  template <uint32_t kOpcode, typename ArgumentsType0, typename OperandType>
  void EmitITypeInstruction(ArgumentsType0&& argument0, OperandType&& operand) {
    return EmitInstruction<kOpcode, 0x0000'707f>(Rd(argument0), Rs1(operand.base), operand.disp);
  }

  // Csr instructions are described as I-type instructions in RISC-V manual, but unlike most
  // I-type instructions they use IImmediate to encode Csr register number and it comes as second
  // argument, not third. In addition Csr value is defined as unsigned and not as signed which
  // means certain Csr values (e.g. kVlenb) wouldn't be accepted as IImmediate!
  template <uint32_t kOpcode, typename ArgumentsType0>
  void EmitITypeInstruction(ArgumentsType0&& argument0, Csr csr, Register argument1) {
    return EmitInstruction<kOpcode, 0x0000'707f>(
        Rd(argument0),
        IImmediate{riscv::RawImmediate{static_cast<int32_t>(csr) << 20}},
        Rs1(argument1));
  }

  template <uint32_t kOpcode, typename ArgumentsType0>
  void EmitITypeInstruction(ArgumentsType0&& argument0, Csr csr, CsrImmediate immediate) {
    return EmitInstruction<kOpcode, 0x0000'707f>(
        Rd(argument0), IImmediate{riscv::RawImmediate{static_cast<int32_t>(csr) << 20}}, immediate);
  }

  template <uint32_t kOpcode,
            typename ArgumentsType0,
            typename ArgumentsType1,
            typename ImmediateType>
  void EmitITypeInstruction(ArgumentsType0&& argument0,
                            ArgumentsType1&& argument1,
                            ImmediateType&& immediate) {
    // Some I-type instructions use immediate as opcode extension. In that case different,
    // smaller, immediate with smaller mask is used. 0xfff0'707f &
    // ~std::decay_t<ImmediateType>::kMask turns these bits that are not used as immediate into
    // parts of opcode. For full I-immediate it produces 0x0000'707f, same as with I-type memory
    // operand.
    return EmitInstruction<kOpcode, 0xfff0'707f & ~std::decay_t<ImmediateType>::kMask>(
        Rd(argument0), Rs1(argument1), immediate);
  }

  template <uint32_t kOpcode, typename ArgumentsType0, typename ImmediateType>
  void EmitJTypeInstruction(ArgumentsType0&& argument0, ImmediateType&& immediate) {
    return EmitInstruction<kOpcode, 0x0000'007f>(Rd(argument0), immediate);
  }

  template <uint32_t kOpcode, typename OperandType>
  void EmitPTypeInstruction(OperandType&& operand) {
    return EmitInstruction<kOpcode, 0x01f0'7fff>(Rs1(operand.base), operand.disp);
  }

  template <uint32_t kOpcode, typename ArgumentsType0, typename ArgumentsType1>
  void EmitRTypeInstruction(ArgumentsType0&& argument0, ArgumentsType1&& argument1) {
    return EmitInstruction<kOpcode, 0xfff0'707f>(Rd(argument0), Rs1(argument1));
  }

  template <uint32_t kOpcode, typename ArgumentsType0, typename ArgumentsType1>
  void EmitRTypeInstruction(ArgumentsType0&& argument0,
                            ArgumentsType1&& argument1,
                            Rounding argument2) {
    return EmitInstruction<kOpcode, 0xfff0'007f>(Rd(argument0), Rs1(argument1), Rm(argument2));
  }

  template <uint32_t kOpcode,
            typename ArgumentsType0,
            typename ArgumentsType1,
            typename ArgumentsType2>
  void EmitRTypeInstruction(ArgumentsType0&& argument0,
                            ArgumentsType1&& argument1,
                            ArgumentsType2&& argument2) {
    return EmitInstruction<kOpcode, 0xfe00'707f>(Rd(argument0), Rs1(argument1), Rs2(argument2));
  }

  template <uint32_t kOpcode, typename ArgumentsType0, typename OperandType>
  void EmitSTypeInstruction(ArgumentsType0&& argument0, OperandType&& operand) {
    return EmitInstruction<kOpcode, 0x0000'707f>(Rs2(argument0), Rs1(operand.base), operand.disp);
  }

  template <uint32_t kOpcode, typename ArgumentsType0, typename ImmediateType>
  void EmitUTypeInstruction(ArgumentsType0&& argument0, ImmediateType&& immediate) {
    return EmitInstruction<kOpcode, 0x0000'007f>(Rd(argument0), immediate);
  }

 private:
  Assembler() = delete;
  Assembler(const Assembler&) = delete;
  Assembler(Assembler&&) = delete;
  void operator=(const Assembler&) = delete;
  void operator=(Assembler&&) = delete;
};

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bcc(Condition cc,
                                                 Register argument1,
                                                 Register argument2,
                                                 const Label& label) {
  if (cc == Condition::kAlways) {
    Jal(zero, label);
    return;
  } else if (cc == Condition::kNever) {
    return;
  }
  CHECK_EQ(0, static_cast<uint8_t>(cc) & 0xf8);
  jumps_.push_back(Jump{&label, pc(), false});
  EmitInstruction<0x0000'0063, 0x0000'007f>(Cond(cc), Rs1(argument1), Rs2(argument2));
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bcc(Condition cc,
                                                 Register argument1,
                                                 Register argument2,
                                                 BImmediate immediate) {
  if (cc == Condition::kAlways) {
    int32_t encoded_immediate_value = immediate.EncodedValue();
    // Maybe better to provide an official interface to convert BImmediate into JImmediate?
    // Most CPUs have uncoditional jump with longer range than condtional one (8086, ARM, RISC-V)
    // or the same one (modern x86), thus such conversion is natural.
    JImmediate jimmediate =
        riscv::RawImmediate{((encoded_immediate_value >> 19) & 0x000f'f000) |
                            ((encoded_immediate_value << 13) & 0x01f0'0000) |
                            (encoded_immediate_value & static_cast<int32_t>(0xfe00'0000))};
    Jal(zero, jimmediate);
    return;
  } else if (cc == Condition::kNever) {
    return;
  }
  CHECK_EQ(0, static_cast<uint8_t>(cc) & 0xf8);
  EmitInstruction<0x0000'0063, 0x0000'007f>(Cond(cc), Rs1(argument1), Rs2(argument2), immediate);
}

#define BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Name, TargetRegister, InstructionType, Opcode) \
  template <typename DerivedAssemblerType>                                                       \
  inline void Assembler<DerivedAssemblerType>::Name(                                             \
      TargetRegister arg0, const Label& label, Register arg2) {                                  \
    CHECK_NE(arg2, x0);                                                                          \
    jumps_.push_back(Jump{&label, pc(), false});                                                 \
    /* First issue auipc to load top 20 bits of difference between pc and target address */      \
    EmitUTypeInstruction<uint32_t{0x0000'0017}>(arg2, UImmediate{0});                            \
    /* The low 12 bite of difference will be encoded in the memory accessing instruction */      \
    Emit##InstructionType##TypeInstruction<uint32_t{Opcode}>(                                    \
        arg0, Operand<Register, InstructionType##Immediate>{.base = arg2});                      \
  }
BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Fld, FpRegister, I, 0x0000'3007)
BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Flw, FpRegister, I, 0x0000'2007)
BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Fsd, FpRegister, S, 0x0000'3027)
BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Fsw, FpRegister, S, 0x0000'2027)
BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Sb, Register, S, 0x0000'0023)
BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Sh, Register, S, 0x0000'1023)
BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION(Sw, Register, S, 0x0000'2023)
#undef BERBERIS_DEFINE_LOAD_OR_STORE_INSTRUCTION

#define BERBERIS_DEFINE_LOAD_INSTRUCTION(Name, Opcode)                                         \
  template <typename DerivedAssemblerType>                                                     \
  inline void Assembler<DerivedAssemblerType>::Name(Register arg0, const Label& label) {       \
    CHECK_NE(arg0, x0);                                                                        \
    jumps_.push_back(Jump{&label, pc(), false});                                               \
    /* First issue auipc to load top 20 bits of difference between pc and target address */    \
    EmitUTypeInstruction<uint32_t{0x0000'0017}>(arg0, UImmediate{0});                          \
    /* The low 12 bite of difference will be encoded in the memory accessing instruction */    \
    EmitITypeInstruction<uint32_t{Opcode}>(arg0, Operand<Register, IImmediate>{.base = arg0}); \
  }
BERBERIS_DEFINE_LOAD_INSTRUCTION(Lb, 0x0000'0003)
BERBERIS_DEFINE_LOAD_INSTRUCTION(Lbu, 0x0000'4003)
BERBERIS_DEFINE_LOAD_INSTRUCTION(Lh, 0x0000'1003)
BERBERIS_DEFINE_LOAD_INSTRUCTION(Lhu, 0x0000'5003)
BERBERIS_DEFINE_LOAD_INSTRUCTION(Lw, 0x0000'2003)
#undef BERBERIS_DEFINE_LOAD_INSTRUCTION

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::La(Register arg0, const Label& label) {
  CHECK_NE(arg0, x0);
  jumps_.push_back(Jump{&label, pc(), false});
  // First issue auipc to load top 20 bits of difference between pc and target address
  EmitUTypeInstruction<uint32_t{0x0000'0017}>(arg0, UImmediate{0});
  // The low 12 bite of difference will be added with addi instruction
  EmitITypeInstruction<uint32_t{0x0000'0013}>(arg0, arg0, IImmediate{0});
}

#define BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION(Name, Opcode)          \
  template <typename DerivedAssemblerType>                             \
  inline void Assembler<DerivedAssemblerType>::Name(                   \
      Register arg0, Register arg1, const Label& label) {              \
    jumps_.push_back(Jump{&label, pc(), false});                       \
    EmitBTypeInstruction<uint32_t{Opcode}>(arg0, arg1, BImmediate{0}); \
  }
BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION(Beq, 0x0000'0063)
BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION(Bge, 0x0000'5063)
BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION(Bgeu, 0x0000'7063)
BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION(Blt, 0x0000'4063)
BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION(Bltu, 0x0000'6063)
BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION(Bne, 0x0000'1063)
#undef BERBERIS_DEFINE_CONDITIONAL_INSTRUCTION

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Jal(Register argument0, const Label& label) {
  jumps_.push_back(Jump{&label, pc(), false});
  EmitInstruction<0x0000'006f, 0x0000'007f>(Rd(argument0));
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::ResolveJumps() {
  for (const auto& jump : jumps_) {
    const Label* label = jump.label;
    uint32_t pc = jump.pc;
    CHECK(label->IsBound());
    if (jump.is_recovery) {
      // Add pc -> label correspondence to recovery map.
      AddRelocation(0, RelocationType::RelocRecoveryPoint, pc, label->position());
    } else {
      int32_t offset = label->position() - pc;
      auto ProcessLabel =
          [this, pc, offset]<typename ImmediateType,
                             std::optional<ImmediateType> (*MakeImmediate)(int32_t)>() {
            auto encoded_immediate = MakeImmediate(offset);
            if (!encoded_immediate.has_value()) {
              // UImmediate means we are dealing with auipc here, means we may accept any
              // ±2GB offset, but need to look at the next instruction to do that.
              if constexpr (std::is_same_v<ImmediateType, UImmediate>) {
                // Bottom immediate is decoded with a 12 → 32 bit sign-extended.
                // Compensate that by adding sign-bit of bottom to top.
                // Make calculation as unsigned types to ensure we wouldn't hit any UB here.
                int32_t top = (static_cast<uint32_t>(offset) +
                               ((static_cast<uint32_t>(offset) & (1U << 11)) * 2)) &
                              0xffff'f000U;
                struct {
                  int32_t data : 12;
                } bottom = {offset};
                *AddrAs<int32_t>(pc) |= UImmediate{top}.EncodedValue();
                *AddrAs<int32_t>(pc + 4) |= ((*AddrAs<int32_t>(pc + 4) & 96) == 32)
                                                ? SImmediate{bottom.data}.EncodedValue()
                                                : IImmediate{bottom.data}.EncodedValue();
                return true;
              }
              return false;
            }
            *AddrAs<int32_t>(pc) |= encoded_immediate->EncodedValue();
            return true;
          };
      // Check the instruction type:
      //   AUIPC uses UImmediate, Jal uses JImmediate, while Bcc uses BImmediate.
      bool RelocationInRange;
      if (*AddrAs<int32_t>(pc) & 16) {
        RelocationInRange = ProcessLabel.template operator()<UImmediate, MakeUImmediate>();
      } else if (*AddrAs<int32_t>(pc) & 4) {
        RelocationInRange = ProcessLabel.template operator()<JImmediate, MakeJImmediate>();
      } else {
        RelocationInRange = ProcessLabel.template operator()<BImmediate, MakeBImmediate>();
      }
      // Maybe need to propagate error to caller?
      CHECK(RelocationInRange);
    }
  }
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Mv(Register dest, Register src) {
  Addi(dest, src, 0);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Li(Register dest, int32_t imm32) {
  // If the value fits into 12bit I-Immediate type, load using addi.
  if (-2048 <= imm32 && imm32 <= 2047) {
    Addi(dest, Assembler::zero, static_cast<IImmediate>(imm32));
  } else {
    // Otherwise we need to use 2 instructions: lui to load top 20 bits and addi for bottom 12 bits,
    // however since the I-Immediate is signed, we could not just split the number into 2 parts: for
    // example loading 4095 should result in loading 1 in upper 20 bits (lui 0x1) and then
    // subtracting 1 (addi dest, dest, -1).
    // Perform calculations on unsigned type to avoid undefined behavior.
    uint32_t uimm = static_cast<uint32_t>(imm32);
    // Since bottom 12bits are loaded via a 12-bit signed immediate, we need to add the sign bit to
    // the top part.
    int32_t top = (uimm + ((uimm & (1U << 11)) << 1)) & 0xffff'f000;
    // Sign extends the bottom 12 bits.
    struct {
      int32_t data : 12;
    } bottom = {imm32};
    Lui(dest, static_cast<UImmediate>(top));
    if (bottom.data) {
      Addi(dest, dest, static_cast<IImmediate>(bottom.data));
    }
  }
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Ret() {
  Jalr(Assembler::x0, Assembler::x1, static_cast<IImmediate>(0));
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Call(const Label& label) {
  jumps_.push_back(Jump{&label, pc(), false});
  // First issue auipc to load top 20 bits of difference between pc and target address
  EmitUTypeInstruction<uint32_t{0x0000'0017}>(Assembler::x6, UImmediate{0});
  // The low 12 bite of difference will be added with jalr instruction
  EmitITypeInstruction<uint32_t{0x0000'0067}>(Assembler::x1, Assembler::x6, IImmediate{0});
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Tail(const Label& label) {
  jumps_.push_back(Jump{&label, pc(), false});
  // First issue auipc to load top 20 bits of difference between pc and target address
  EmitUTypeInstruction<uint32_t{0x0000'0017}>(Assembler::x6, UImmediate{0});
  // The low 12 bite of difference will be added with jalr instruction
  EmitITypeInstruction<uint32_t{0x0000'0067}>(Assembler::x0, Assembler::x6, IImmediate{0});
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bgt(Register arg0, Register arg1, const Label& label) {
  Blt(arg1, arg0, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bgtu(Register arg0,
                                                  Register arg1,
                                                  const Label& label) {
  Bltu(arg1, arg0, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Ble(Register arg0, Register arg1, const Label& label) {
  Bge(arg1, arg0, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bleu(Register arg0,
                                                  Register arg1,
                                                  const Label& label) {
  Bgeu(arg1, arg0, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Beqz(Register arg0, const Label& label) {
  Beq(arg0, zero, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bnez(Register arg0, const Label& label) {
  Bne(arg0, zero, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Blez(Register arg0, const Label& label) {
  Ble(arg0, zero, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bgez(Register arg0, const Label& label) {
  Bge(arg0, zero, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bltz(Register arg0, const Label& label) {
  Blt(arg0, zero, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Bgtz(Register arg0, const Label& label) {
  Bgt(arg0, zero, label);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Seqz(Register arg0, Register arg1) {
  Sltiu(arg0, arg1, static_cast<IImmediate>(1));
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Snez(Register arg0, Register arg1) {
  Sltu(arg0, zero, arg1);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Sltz(Register arg0, Register arg1) {
  Slt(arg0, arg1, zero);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Sgtz(Register arg0, Register arg1) {
  Slt(arg0, zero, arg1);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::J(JImmediate arg0) {
  Jal(zero, arg0);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Jal(JImmediate arg0) {
  Jal(x1, arg0);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Jr(Register arg0) {
  Jalr(zero, arg0, 0);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Jalr(Register arg0) {
  Jalr(x1, arg0, 0);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Not(Register arg0, Register arg1) {
  Xori(arg0, arg1, -1);
}

template <typename DerivedAssemblerType>
inline void Assembler<DerivedAssemblerType>::Neg(Register arg0, Register arg1) {
  Sub(arg0, zero, arg1);
}

}  // namespace riscv

}  // namespace berberis

#endif  // BERBERIS_ASSEMBLER_COMMON_RISCV_H_
