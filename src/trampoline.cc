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

#include "trampoline.h"

#define protected public
#include "berberis/assembler/rv64i.h"
#undef protected

namespace rv64hook {

using berberis::rv64i::Assembler;

static constexpr uint16_t kWideTrampoline[] = {
    // auipc t3, 0
    0x0e17,
    0x0000,
    // ld t3, 10(t3)
    0x3e03,
    0x00ae,
    // jr t3
    0x8e02,
};

void Write32BitJumpInstruction(uint32_t op, func_t address, uint32_t v) {
  auto base = v >> 12;
  auto add = static_cast<int32_t>(v & ~0xFFFFF000);
  if (add >= 0x800) {
    ++base;
    add = -(0x1000 - add);
  }

  uint32_t code[2];
  {
    Assembler::RegisterOperand<Assembler::RdMarker, Assembler::Register> rd(Assembler::t3);
    Assembler::UImmediate imm(base << 12);
    code[0] = op | rd.EncodeImmediate() | imm.EncodedValue();
  }
  {
    Assembler::RegisterOperand<Assembler::RdMarker, Assembler::Register> rd(Assembler::zero);
    Assembler::RegisterOperand<Assembler::Rs1Marker, Assembler::Register> rs1(Assembler::t3);
    Assembler::IImmediate imm(add);
    code[1] = 0x67 | rd.EncodeImmediate() | rs1.EncodeImmediate() | imm.EncodedValue();
  }
  *reinterpret_cast<void**>(address) = *reinterpret_cast<void**>(code);
}

TrampolineType GetSuggestedTrampolineType(func_t address, void* target) {
  auto a = reinterpret_cast<uintptr_t>(address);
  auto b = reinterpret_cast<uintptr_t>(target);
  auto off = static_cast<intptr_t>(b - a);

  if (off >= -0xFFFFE && off <= 0xFFFFE) {
    return TrampolineType::kPC20;
  } else if (b < 0xFFFFF800) {
    return TrampolineType::kVA32;
  } else if (off > 0 && off < 0xFFFFF800) {
    return TrampolineType::kPC32;
  } else return TrampolineType::kWide;
}

int GetFirstTrampolineSize(TrampolineType type) {
  switch (type) {
    case TrampolineType::kPC20:
      return 4;
    case TrampolineType::kPC32:
    case TrampolineType::kVA32:
      return 8;
    default:
      return sizeof(kWideTrampoline) + sizeof(void*);
  }
}

void WriteFirstTrampoline(func_t address, void* target, TrampolineType type) {
  size_t size = 0;
  switch (type) {
    case TrampolineType::kPC20: {
      // jal zero, xxx
      size = 4;
      auto off = reinterpret_cast<intptr_t>(target) - reinterpret_cast<intptr_t>(address);
      if (off >= -0xFFFFE && off <= 0xFFFFE) {
        Assembler::RegisterOperand<Assembler::RdMarker, Assembler::Register> rd(Assembler::zero);
        Assembler::JImmediate imm(off);
        auto code = reinterpret_cast<uint32_t*>(address);
        code[0] = 0x6f | rd.EncodeImmediate() | imm.EncodedValue();
      } else __builtin_unreachable();
    } break;
    case TrampolineType::kPC32: {
      // auipc t3, xxx
      // jalr zero, t3, xxx
      size = 8;
      auto off = reinterpret_cast<intptr_t>(target) - reinterpret_cast<intptr_t>(address);
      if (off > 0 && off < 0xFFFFF800) {
        Write32BitJumpInstruction(0x17, address, static_cast<uint32_t>(off));
      } else __builtin_unreachable();
    } break;
    case TrampolineType::kVA32: {
      // lui t3, xxx
      // jalr zero, t3, xxx
      size = 8;
      auto addr = reinterpret_cast<uintptr_t>(target);
      if (addr < 0xFFFFF800) {
        Write32BitJumpInstruction(0x37, address, static_cast<uint32_t>(addr));
      } else __builtin_unreachable();
    } break;
    default: {
      // auipc t3, 0
      // ld t3, 10(t3)
      // jr t3
      // .quad xxx
      size = sizeof(kWideTrampoline) + sizeof(void*);
      *reinterpret_cast<uintptr_t*>(address) = *reinterpret_cast<const uintptr_t*>(kWideTrampoline);
      *reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(address) + 8) = kWideTrampoline[4];
      *reinterpret_cast<func_t*>(static_cast<uint8_t*>(address) + sizeof(kWideTrampoline)) = target;
    }
  }
  __builtin___clear_cache(static_cast<char*>(address), static_cast<char*>(address) + size);
}

}  // namespace rv64hook
