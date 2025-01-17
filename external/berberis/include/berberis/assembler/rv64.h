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

// Assembler to produce RV64 instructions (no ABI version). Somewhat influenced by V8 assembler.

#ifndef BERBERIS_ASSEMBLER_RV64_H_
#define BERBERIS_ASSEMBLER_RV64_H_

#include <bit>          // std::countr_zero
#include <type_traits>  // std::is_same

#include "berberis/assembler/riscv.h"

namespace berberis::rv64 {

class Assembler : public riscv::Assembler<Assembler> {
 public:
  using BaseAssembler = riscv::Assembler<Assembler>;
  using FinalAssembler = Assembler;

  explicit Assembler(MachineCode* code) : BaseAssembler(code) {}

  using ShiftImmediate = BaseAssembler::Shift64Immediate;

  // Don't use templates here to enable implicit conversions.
#define BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(IntType)                                \
  static constexpr std::optional<ShiftImmediate> MakeShiftImmediate(IntType value) { \
    return BaseAssembler::MakeShift64Immediate(value);                               \
  }
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(int8_t)
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(uint8_t)
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(int16_t)
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(uint16_t)
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(int32_t)
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(uint32_t)
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(int64_t)
  BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE(uint64_t)
#undef BERBERIS_DEFINE_MAKE_SHIFT_IMMEDIATE

  friend BaseAssembler;

// Instructions.
#include "berberis/assembler/gen_assembler_rv64-inl.h"  // NOLINT generated file!

 private:
  Assembler() = delete;
  Assembler(const Assembler&) = delete;
  Assembler(Assembler&&) = delete;
  void operator=(const Assembler&) = delete;
  void operator=(Assembler&&) = delete;
  void Li32(Register dest, int32_t imm32);
};

inline void Assembler::Ld(Register arg0, const Label& label) {
  jumps_.push_back(Jump{&label, pc(), false});
  // First issue auipc to load top 20 bits of difference between pc and target address
  EmitUTypeInstruction<uint32_t{0x0000'0017}>(arg0, UImmediate{0});
  // The low 12 bite of difference will be encoded in the Ld instruction
  EmitITypeInstruction<uint32_t{0x0000'3003}>(arg0, Operand<Register, IImmediate>{.base = arg0});
}

// It's needed to unhide 32bit immediate version.
inline void Assembler::Li32(Register dest, int32_t imm32) {
  BaseAssembler::Li(dest, imm32);
};

inline void Assembler::Li(Register dest, int64_t imm64) {
  int32_t imm32 = static_cast<int32_t>(imm64);
  if (static_cast<int64_t>(imm32) == imm64) {
    Li32(dest, imm32);
  } else {
    // Perform calculations on unsigned type to avoid undefined behavior.
    uint64_t uimm = static_cast<uint64_t>(imm64);
    if (imm64 & 0xfff) {
      // Since bottom 12bits are loaded via a 12-bit signed immediate, we need to transfer the sign
      // bit to the top part.
      int64_t top = (uimm + ((uimm & (1ULL << 11)) << 1)) & 0xffff'ffff'ffff'f000;
      // Sign extends the bottom 12 bits.
      struct {
        int64_t data : 12;
      } bottom = {imm64};
      Li(dest, top);
      Addi(dest, dest, static_cast<IImmediate>(bottom.data));
    } else {
      uint8_t zeros = std::countr_zero(uimm);
      Li(dest, imm64 >> zeros);
      Slli(dest, dest, static_cast<Shift64Immediate>(zeros));
    }
  }
}

inline void Assembler::Lwu(Register arg0, const Label& label) {
  jumps_.push_back(Jump{&label, pc(), false});
  // First issue auipc to load top 20 bits of difference between pc and target address
  EmitUTypeInstruction<uint32_t{0x0000'0017}>(arg0, UImmediate{0});
  // The low 12 bite of difference will be encoded in the Lwu instruction
  EmitITypeInstruction<uint32_t{0x0000'6003}>(arg0, Operand<Register, IImmediate>{.base = arg0});
}

inline void Assembler::Sd(Register arg0, const Label& label, Register arg2) {
  jumps_.push_back(Jump{&label, pc(), false});
  // First issue auipc to load top 20 bits of difference between pc and target address
  EmitUTypeInstruction<uint32_t{0x0000'0017}>(arg2, UImmediate{0});
  // The low 12 bite of difference will be encoded in the Sd instruction
  EmitSTypeInstruction<uint32_t{0x0000'3023}>(arg0, Operand<Register, SImmediate>{.base = arg2});
}

inline void Assembler::SextW(Register arg0, Register arg1) {
  Addiw(arg0, arg1, 0);
}

inline void Assembler::ZextW(Register arg0, Register arg1) {
  AddUW(arg0, arg1, zero);
}

inline void Assembler::Negw(Register arg0, Register arg1) {
  Subw(arg0, zero, arg1);
}

}  // namespace berberis::rv64

#endif  // BERBERIS_ASSEMBLER_RV64_H_
