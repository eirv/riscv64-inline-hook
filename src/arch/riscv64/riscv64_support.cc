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

#include "arch/common/asm.h"
#include "arch/common/trampoline.h"
#include "arch/riscv64/riscv64_relocator.h"
#include "config.h"
#include "core/memory.h"

namespace rv64hook {

size_t InstructionRelocator::Relocate(const void* address, int size, void** relocated) {
  return RV64Relocator::Relocate(static_cast<const uint16_t*>(address), size, relocated);
}

bool Trampoline::IsValid(TrampolineType type) {
  switch (type) {
    case TrampolineType::kWide:
    case TrampolineType::kPC20:
    case TrampolineType::kPC32:
    case TrampolineType::kVA32:
      return true;
    default:
      return false;
  }
}

bool Trampoline::Write32BitJumpInstruction(uint32_t op, func_t address, uint32_t v) {
  auto base = v >> 12;
  auto add = static_cast<int32_t>(v & ~0xFFFFF000);
  if (add >= 0x800) {
    ++base;
    add = -(0x1000 - add);
  }

  uint32_t code[2];
  {
    Assembler::RegisterOperand<Assembler::RdMarker, Assembler::Register> rd(
        Assembler::TMP_GENERIC_REGISTER);
    Assembler::UImmediate imm(base << 12);
    code[0] = op | rd.EncodeImmediate() | imm.EncodedValue();
  }
  {
    Assembler::RegisterOperand<Assembler::RdMarker, Assembler::Register> rd(Assembler::zero);
    Assembler::RegisterOperand<Assembler::Rs1Marker, Assembler::Register> rs1(
        Assembler::TMP_GENERIC_REGISTER);
    Assembler::IImmediate imm(add);
    code[1] = 0x67 | rd.EncodeImmediate() | rs1.EncodeImmediate() | imm.EncodedValue();
  }
  return Memory::Copy(address, code, sizeof(code));
}

TrampolineType Trampoline::GetSuggestedTrampolineType(func_t address, void* target) {
  auto a = reinterpret_cast<uintptr_t>(address);
  auto b = reinterpret_cast<uintptr_t>(target);
  auto off = static_cast<intptr_t>(b - a);

  if (off >= -0xFFFFE && off <= 0xFFFFE) {
    return TrampolineType::kPC20;
  } else if (b < 0x7FFFF800) {
    return TrampolineType::kVA32;
  } else if (off > 0 && off < 0x7FFFF800) {
    return TrampolineType::kPC32;
  } else return TrampolineType::kWide;
}

class [[gnu::packed]] WideTrampoline {
 public:
  inline WideTrampoline(void* address) : address_(address) {
  }

 private:
  [[maybe_unused]] uint32_t auipc_ = 0x00000e17;  // auipc t3, 0
  [[maybe_unused]] uint32_t load_ = 0x00ae3e03;   // ld t3, 10(t3)
  [[maybe_unused]] uint16_t jump_ = 0x8e02;       // jr t3
  [[maybe_unused]] void* address_;                // .quad xxx
};

int Trampoline::GetFirstTrampolineSize(TrampolineType type) {
  switch (type) {
    case TrampolineType::kPC20:
      return 4;
    case TrampolineType::kPC32:
    case TrampolineType::kVA32:
      return 8;
    default:
      return sizeof(WideTrampoline);
  }
}

bool Trampoline::WriteFirstTrampoline(func_t address, void* target, TrampolineType type) {
  size_t size = 0;
  bool copied = false;

  switch (type) {
    case TrampolineType::kPC20: {
      // jal zero, xxx
      size = 4;
      auto off = reinterpret_cast<intptr_t>(target) - reinterpret_cast<intptr_t>(address);
      if (off >= -0xFFFFE && off <= 0xFFFFE) {
        Assembler::RegisterOperand<Assembler::RdMarker, Assembler::Register> rd(Assembler::zero);
        Assembler::JImmediate imm(off);
        uint32_t code = 0x6f | rd.EncodeImmediate() | imm.EncodedValue();
        copied = Memory::Copy(address, &code, sizeof(code));
      } else abort();
    } break;

    case TrampolineType::kPC32: {
      // auipc t3, xxx
      // jalr zero, t3, xxx
      size = 8;
      auto off = reinterpret_cast<intptr_t>(target) - reinterpret_cast<intptr_t>(address);
      if (off > 0 && off < 0x7FFFF800) {
        copied = Write32BitJumpInstruction(0x17, address, static_cast<uint32_t>(off));
      } else abort();
    } break;

    case TrampolineType::kVA32: {
      // lui t3, xxx
      // jalr zero, t3, xxx
      size = 8;
      auto addr = reinterpret_cast<uintptr_t>(target);
      if (addr < 0x7FFFF800) {
        copied = Write32BitJumpInstruction(0x37, address, static_cast<uint32_t>(addr));
      } else abort();
    } break;

    default: {
      WideTrampoline trampoline(target);
      copied = Memory::Copy(address, &trampoline, sizeof(trampoline));
    }
  }
  __builtin___clear_cache(static_cast<char*>(address), static_cast<char*>(address) + size);
  return copied;
}

std::tuple<void*, bool> Trampoline::AllocSecondTrampoline(func_t address) {
  auto [code, code_size] = GetSecondTrampoline();
  auto size = code_size + sizeof(TrampolineData);
  uintptr_t start = 0, end = 0;

  bool is_user_alloc = false;
  void* trampoline = nullptr;
  auto ta = GetTrampolineAllocator();

  switch (ta->type) {
    case TrampolineType::kPC20:
      start = reinterpret_cast<uintptr_t>(address) - 0xFFFFE;
      end = reinterpret_cast<uintptr_t>(address) + 0xFFFFE;
      break;
    case TrampolineType::kPC32:
      start = reinterpret_cast<uintptr_t>(address);
      end = reinterpret_cast<uintptr_t>(address) + 0x7FFFF800;
      break;
    case TrampolineType::kVA32:
      end = 0x7FFFF800;
      break;
    case TrampolineType::kCustom:
      trampoline = ta->custom_alloc(address, size, ta->data);
      is_user_alloc = trampoline != nullptr;
      break;
    default:
      break;
  }

  if (!trampoline) {
    if (start || end) {
      trampoline = Memory::Alloc(size, start, end);
      if (!trampoline) [[unlikely]] {
        trampoline = Memory::Alloc(size);
      }
    } else {
      trampoline = Memory::Alloc(size);
    }
    if (!trampoline) [[unlikely]] {
      return {};
    }
  }

  ScopedWritableAllocatedMemory unused(is_user_alloc ? nullptr : trampoline);
  memcpy(trampoline, code, code_size);
  memset(static_cast<uint8_t*>(trampoline) + code_size, 0, sizeof(TrampolineData));
  __builtin___clear_cache(static_cast<char*>(trampoline),
                          static_cast<char*>(trampoline) + code_size);

  return {trampoline, is_user_alloc};
}

TrampolineData* Trampoline::GetTrampolineData(void* trampoline) {
  return reinterpret_cast<TrampolineData*>(static_cast<uint8_t*>(trampoline) +
                                           std::get<1>(GetSecondTrampoline()));
}

extern "C" void ASM_LABEL(trampoline)();
extern "C" void ASM_LABEL(trampoline_end)();

std::tuple<const void*, size_t> Trampoline::GetSecondTrampoline() {
#ifdef RV64HOOK_BUILD_TRAMPOLINE
  return {reinterpret_cast<const void*>(ASM_LABEL(trampoline)),
          reinterpret_cast<size_t>(ASM_LABEL(trampoline_end)) -
              reinterpret_cast<size_t>(ASM_LABEL(trampoline))};
#else
  static constexpr uint16_t kTrampoline[] = {
      0x0e17, 0x0000, 0x0e03, 0x312e, 0x0963, 0x000e, 0x0e17, 0x0000, 0x3e03, 0x2e0e, 0x0863,
      0x000e, 0x8e02, 0x0e17, 0x0000, 0x3e03, 0x2dae, 0x8e02, 0x3423, 0xe021, 0x0113, 0xdf81,
      0xe406, 0xec0e, 0xf012, 0xf416, 0xf81a, 0xfc1e, 0xe0a2, 0xe4a6, 0xe8aa, 0xecae, 0xf0b2,
      0xf4b6, 0xf8ba, 0xfcbe, 0xe142, 0xe546, 0xe94a, 0xed4e, 0xf152, 0xf556, 0xf95a, 0xfd5e,
      0xe1e2, 0xe5e6, 0xe9ea, 0xedee, 0xf1f2, 0xf5f6, 0xf9fa, 0xfdfe, 0xa202, 0xa606, 0xaa0a,
      0xae0e, 0xb212, 0xb616, 0xba1a, 0xbe1e, 0xa2a2, 0xa6a6, 0xaaaa, 0xaeae, 0xb2b2, 0xb6b6,
      0xbaba, 0xbebe, 0xa342, 0xa746, 0xab4a, 0xaf4e, 0xb352, 0xb756, 0xbb5a, 0xbf5e, 0xa3e2,
      0xa7e6, 0xabea, 0xafee, 0xb3f2, 0xb7f6, 0xbbfa, 0xbffe, 0x3023, 0x2001, 0x0597, 0x0000,
      0xb583, 0x2385, 0xbe03, 0x0205, 0xe072, 0x8e03, 0x0505, 0x0963, 0x000e, 0xbe03, 0x0305,
      0x0563, 0x000e, 0x0028, 0x61b0, 0x9e02, 0x6582, 0xf1ed, 0x0e17, 0x0000, 0x3e03, 0x230e,
      0x0963, 0x000e, 0x0517, 0x0000, 0x2503, 0x22c5, 0x65a2, 0x9e02, 0xa801, 0xf057, 0xcd80,
      0x3e57, 0x5e00, 0x60a2, 0xce57, 0x5e00, 0x0e03, 0x2001, 0x3ffe, 0x3f5e, 0x3ebe, 0x3e1e,
      0x2dfe, 0x2d5e, 0x2cbe, 0x2c1e, 0x3bfa, 0x3b5a, 0x3aba, 0x3a1a, 0x29fa, 0x295a, 0x28ba,
      0x281a, 0x37f6, 0x3756, 0x36b6, 0x3616, 0x25f6, 0x2556, 0x24b6, 0x2416, 0x33f2, 0x3352,
      0x32b2, 0x3212, 0x21f2, 0x2152, 0x20b2, 0x2012, 0x7fee, 0x7f4e, 0x7eae, 0x7e0e, 0x6dee,
      0x6d4e, 0x6cae, 0x6c0e, 0x7bea, 0x7b4a, 0x7aaa, 0x7a0a, 0x69ea, 0x694a, 0x68aa, 0x680a,
      0x77e6, 0x7746, 0x76a6, 0x7606, 0x65e6, 0x6546, 0x64a6, 0x6406, 0x73e2, 0x7342, 0x72a2,
      0x7202, 0x61e2, 0x60a2, 0x6142, 0x1263, 0x160e, 0x0e17, 0x0000, 0x1e03, 0x18ee, 0x08e3,
      0xe80e, 0x0e17, 0x0000, 0x3e03, 0x166e, 0x9e02, 0x3423, 0xe021, 0x0113, 0xdf81, 0xec0e,
      0xf012, 0xf416, 0xf81a, 0xfc1e, 0xe0a2, 0xe4a6, 0xe8aa, 0xecae, 0xf0b2, 0xf4b6, 0xf8ba,
      0xfcbe, 0xe142, 0xe546, 0xe94a, 0xed4e, 0xf152, 0xf556, 0xf95a, 0xfd5e, 0xe1e2, 0xe5e6,
      0xe9ea, 0xedee, 0xf1f2, 0xf5f6, 0xf9fa, 0xfdfe, 0xa202, 0xa606, 0xaa0a, 0xae0e, 0xb212,
      0xb616, 0xba1a, 0xbe1e, 0xa2a2, 0xa6a6, 0xaaaa, 0xaeae, 0xb2b2, 0xb6b6, 0xbaba, 0xbebe,
      0xa342, 0xa746, 0xab4a, 0xaf4e, 0xb352, 0xb756, 0xbb5a, 0xbf5e, 0xa3e2, 0xa7e6, 0xabea,
      0xafee, 0xb3f2, 0xb7f6, 0xbbfa, 0xbffe, 0x0e17, 0x0000, 0x3e03, 0x0e2e, 0x0963, 0x000e,
      0x0517, 0x0000, 0x2503, 0x0e65, 0x9e02, 0xe42a, 0xa029, 0x0e13, 0x0081, 0x0e27, 0x020e,
      0x0597, 0x0000, 0xb583, 0x0a85, 0xbe03, 0x0205, 0xe072, 0x8e03, 0x0505, 0x0963, 0x000e,
      0xbe03, 0x0385, 0x0563, 0x000e, 0x0028, 0x61b0, 0x9e02, 0x6582, 0xf1ed, 0x3ffe, 0x3f5e,
      0x3ebe, 0x3e1e, 0x2dfe, 0x2d5e, 0x2cbe, 0x2c1e, 0x3bfa, 0x3b5a, 0x3aba, 0x3a1a, 0x29fa,
      0x295a, 0x28ba, 0x281a, 0x37f6, 0x3756, 0x36b6, 0x3616, 0x25f6, 0x2556, 0x24b6, 0x2416,
      0x33f2, 0x3352, 0x32b2, 0x3212, 0x21f2, 0x2152, 0x20b2, 0x2012, 0x7fee, 0x7f4e, 0x7eae,
      0x7e0e, 0x6dee, 0x6d4e, 0x6cae, 0x6c0e, 0x7bea, 0x7b4a, 0x7aaa, 0x7a0a, 0x69ea, 0x694a,
      0x68aa, 0x680a, 0x77e6, 0x7746, 0x76a6, 0x7606, 0x65e6, 0x6546, 0x64a6, 0x6406, 0x73e2,
      0x7342, 0x72a2, 0x7202, 0x61e2, 0x60a2, 0x6142, 0x8082,
  };
  return {kTrampoline, sizeof(kTrampoline)};
#endif
}

}  // namespace rv64hook
