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

#include "arch/common/instruction_relocator.h"
#include "berberis/assembler/rv64i.h"
#include "berberis/decoder/riscv64/decoder.h"
#include "config.h"
#include "core/logger.h"
#include "core/memory.h"

namespace rv64hook {

class Assembler : public berberis::rv64i::Assembler {
 public:
  explicit inline Assembler(berberis::MachineCode* code) : berberis::rv64i::Assembler(code) {
  }

  friend class Trampoline;
};

class RV64Relocator {
 public:
  using Decoder = berberis::Decoder<RV64Relocator>;
  using Register = Assembler::Register;

  static size_t Relocate(const uint16_t* address, int size, void** relocated) {
    berberis::MachineCode code;
    Assembler assembler(&code);

    RV64Relocator relocator(assembler);
    Decoder decoder(&relocator);

    size_t overwrite_size = 0;
    while (size > 0) {
      relocator.SetPC(reinterpret_cast<uintptr_t>(address));
      auto count = decoder.Decode(address);
      if (auto state = relocator.GetState(); state == kSkipped) {
        if (count == 4) {
          assembler.TwoByte(address[0], address[1]);
        } else {
          assembler.TwoByte(address[0]);
        }
      } else if (state != kRelocated) {
        return 0;
      }
      size -= count;
      address += count / sizeof(uint16_t);
      overwrite_size += count;
    }

    relocator.JumpAddress(reinterpret_cast<uint64_t>(address));
    relocator.EmitAddresses();
    assembler.Finalize();

    auto backup = Memory::Alloc(code.install_size());
    *relocated = backup;

    berberis::RecoveryMap recovery_map;
    ScopedWritableAllocatedMemory unused(backup);
    code.InstallUnsafe(static_cast<uint8_t*>(backup), &recovery_map);
    __builtin___clear_cache(static_cast<char*>(backup),
                            static_cast<char*>(backup) + code.install_size());

    return overwrite_size;
  }

  void Auipc(const typename Decoder::UpperImmArgs& args) {
    assembler_.Ld(Register{args.dst}, addresses_[GetPC() + args.imm]);
    SetNewState(kRelocated);
  }

  void CompareAndBranch(const typename Decoder::BranchArgs& args) {
    auto address = GetPC() + args.offset;
    auto& label = *assembler_.MakeLabel();
    auto& branch_label = *assembler_.MakeLabel();
    switch (args.opcode) {
      case Decoder::BranchOpcode::kBeq:
        if (args.src2 == 0) {
          assembler_.Beqz(Register{args.src1}, branch_label);
        } else {
          assembler_.Beq(Register{args.src1}, Register{args.src2}, branch_label);
        }
        break;
      case Decoder::BranchOpcode::kBne:
        if (args.src2 == 0) {
          assembler_.Bnez(Register{args.src1}, branch_label);
        } else {
          assembler_.Bne(Register{args.src1}, Register{args.src2}, branch_label);
        }
        break;
      case Decoder::BranchOpcode::kBlt:
        if (args.src2 == 0) {
          assembler_.Bltz(Register{args.src1}, branch_label);
        } else {
          assembler_.Blt(Register{args.src1}, Register{args.src2}, branch_label);
        }
        break;
      case Decoder::BranchOpcode::kBge:
        if (args.src2 == 0) {
          assembler_.Bgez(Register{args.src1}, branch_label);
        } else {
          assembler_.Bge(Register{args.src1}, Register{args.src2}, branch_label);
        }
        break;
      case Decoder::BranchOpcode::kBltu:
        assembler_.Bltu(Register{args.src1}, Register{args.src2}, branch_label);
        break;
      case Decoder::BranchOpcode::kBgeu:
        assembler_.Bgeu(Register{args.src1}, Register{args.src2}, branch_label);
        break;
      default:
        Undefined();
        return;
    }
    assembler_.Jal(Assembler::zero, label);
    assembler_.Bind(&branch_label);
    JumpAddress(address);
    assembler_.Bind(&label);
    SetNewState(kRelocated);
  }

  void JumpAndLink(const typename Decoder::JumpAndLinkArgs& args) {
    assembler_.Ld(Assembler::TMP_GENERIC_REGISTER, addresses_[GetPC() + args.offset]);
    assembler_.Jalr(Register{args.dst}, Assembler::TMP_GENERIC_REGISTER, 0);
    SetNewState(kRelocated);
  }

  void JumpAndLinkRegister(const typename Decoder::JumpAndLinkRegisterArgs& args) {
    if (args.dst == 0 && args.base == 1 && args.offset == 0) {
      returned_ = true;
    }
    Skip();
  }

  void Amo(const auto&) {
    Skip();
  }

  void Csr(const auto&) {
    Skip();
  }

  void Fcvt(const auto&) {
    Skip();
  }

  void Fma(const auto&) {
    Skip();
  }

  void Fence(const auto&) {
    Skip();
  }

  void FenceI(const auto&) {
    Skip();
  }

  void Load(const auto&) {
    Skip();
  }

  void Lui(const auto&) {
    Skip();
  }

  void Nop() {
    Skip();
  }

  void Op(auto&&) {
    Skip();
  }

  void OpSingleInput(const auto&) {
    Skip();
  }

  void OpFp(const auto&) {
    Skip();
  }

  void OpFpGpRegisterTargetNoRounding(const auto&) {
    Skip();
  }

  void OpFpGpRegisterTargetSingleInputNoRounding(const auto&) {
    Skip();
  }

  void OpFpNoRounding(const auto&) {
    Skip();
  }

  void FmvFloatToInteger(const auto&) {
    Skip();
  }

  void FmvIntegerToFloat(const auto&) {
    Skip();
  }

  void OpFpSingleInput(const auto&) {
    Skip();
  }

  void OpFpSingleInputNoRounding(const auto&) {
    Skip();
  }

  void OpImm(auto&&) {
    Skip();
  }

  void OpVector(const auto&) {
    Skip();
  }

  void Vsetivli(const auto&) {
    Skip();
  }

  void Vsetvl(const auto&) {
    Skip();
  }

  void Vsetvli(const auto&) {
    Skip();
  }

  void Store(const auto&) {
    Skip();
  }

  void System(const auto&) {
    Skip();
  }

  void Undefined() {
    if (returned_) {
      Skip();
    } else {
      SET_ERROR("Unknown instruction at %#llx", pc_);
      SetNewState(kError);
    }
  }

 private:
  static constexpr const char* kTag = "Instruction Relocator";

  static constexpr uint8_t kNone = 0;
  static constexpr uint8_t kSkipped = 1;
  static constexpr uint8_t kRelocated = 2;
  static constexpr uint8_t kError = 3;

  Assembler& assembler_;
  std::map<uint64_t, Assembler::Label> addresses_{};
  uintptr_t pc_{};
  uint8_t state_{};
  bool returned_{};

  explicit RV64Relocator(Assembler& assembler) : assembler_(assembler) {
  }

  void SetPC(uintptr_t pc) {
    pc_ = pc;
  }

  [[nodiscard]] uintptr_t GetPC() const {
    return pc_;
  }

  void Skip() {
    SetNewState(kSkipped);
  }

  uint8_t GetState() {
    auto s = state_;
    state_ = kNone;
    return s;
  }

  void SetNewState(uint8_t state) {
    if (state_ == kNone) {
      state_ = state;
    } else {
      state_ = kError;
    }
  }

  void JumpAddress(uint64_t address) {
    assembler_.Ld(Assembler::TMP_GENERIC_REGISTER, addresses_[address]);
    assembler_.Jr(Assembler::TMP_GENERIC_REGISTER);
  }

  void EmitAddresses() {
    for (auto& p : addresses_) {
      assembler_.Bind(&p.second);
      assembler_.Emit64(p.first);
    }
  }
};

}  // namespace rv64hook
