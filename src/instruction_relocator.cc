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

#include "instruction_relocator.h"

#include <optional>

#include "berberis/assembler/rv64i.h"
#include "berberis/decoder/riscv64/decoder.h"
#include "logger.h"
#include "memory_allocator.h"

namespace rv64hook {

class RV64Relocator {
 public:
  using Decoder = berberis::Decoder<RV64Relocator>;
  using Assembler = berberis::rv64i::Assembler;
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
      } else if (state == kError) {
        return 0;
      }
      size -= count;
      address += count / sizeof(uint16_t);
      overwrite_size += count;
    }

    relocator.JumpAddress(reinterpret_cast<uint64_t>(address));
    relocator.EmitAddresses();
    assembler.Finalize();

    auto backup = MemoryAllocator::GetDefault()->Alloc(code.install_size());
    *relocated = backup;

    berberis::RecoveryMap recovery_map;
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
    assembler_.Ld(Assembler::t3, addresses_[GetPC() + args.offset]);
    assembler_.Jalr(Register{args.dst}, Assembler::t3, 0);
    SetNewState(kRelocated);
  }

  void JumpAndLinkRegister(const typename Decoder::JumpAndLinkRegisterArgs& args) {
    if (args.dst == 0 && args.base == 1 && args.offset == 0) {
      returned_ = true;
    }
    Skip();
  }

  void Amo(const typename Decoder::AmoArgs&) {
    Skip();
  }

  void Csr(const typename Decoder::CsrArgs&) {
    Skip();
  }

  void Csr(const typename Decoder::CsrImmArgs&) {
    Skip();
  }

  void Fcvt(const typename Decoder::FcvtFloatToFloatArgs&) {
    Skip();
  }

  void Fcvt(const typename Decoder::FcvtFloatToIntegerArgs&) {
    Skip();
  }

  void Fcvt(const typename Decoder::FcvtIntegerToFloatArgs&) {
    Skip();
  }

  void Fma(const typename Decoder::FmaArgs&) {
    Skip();
  }

  void Fence(const typename Decoder::FenceArgs&) {
    Skip();
  }

  void FenceI(const typename Decoder::FenceIArgs&) {
    Skip();
  }

  void Load(const typename Decoder::LoadArgs&) {
    Skip();
  }

  void Load(const typename Decoder::LoadFpArgs&) {
    Skip();
  }

  void Lui(const typename Decoder::UpperImmArgs&) {
    Skip();
  }

  void Nop() {
    Skip();
  }

  template <typename OpArgs>
  void Op(OpArgs&&) {
    Skip();
  }

  void OpSingleInput(const typename Decoder::OpSingleInputArgs&) {
    Skip();
  }

  void OpFp(const typename Decoder::OpFpArgs&) {
    Skip();
  }

  void OpFpGpRegisterTargetNoRounding(const typename Decoder::OpFpGpRegisterTargetNoRoundingArgs&) {
    Skip();
  }

  void OpFpGpRegisterTargetSingleInputNoRounding(
      const typename Decoder::OpFpGpRegisterTargetSingleInputNoRoundingArgs&) {
    Skip();
  }

  void OpFpNoRounding(const typename Decoder::OpFpNoRoundingArgs&) {
    Skip();
  }

  void FmvFloatToInteger(const typename Decoder::FmvFloatToIntegerArgs&) {
    Skip();
  }

  void FmvIntegerToFloat(const typename Decoder::FmvIntegerToFloatArgs&) {
    Skip();
  }

  void OpFpSingleInput(const typename Decoder::OpFpSingleInputArgs&) {
    Skip();
  }

  void OpFpSingleInputNoRounding(const typename Decoder::OpFpSingleInputNoRoundingArgs&) {
    Skip();
  }

  template <typename OpImmArgs>
  void OpImm(OpImmArgs&&) {
    Skip();
  }

  void OpVector(const typename Decoder::VLoadIndexedArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VLoadStrideArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VLoadUnitStrideArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VOpFVfArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VOpFVvArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VOpIViArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VOpIVvArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VOpMVvArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VOpIVxArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VOpMVxArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VStoreIndexedArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VStoreStrideArgs&) {
    Skip();
  }

  void OpVector(const typename Decoder::VStoreUnitStrideArgs&) {
    Skip();
  }

  void Vsetivli(const typename Decoder::VsetivliArgs&) {
    Skip();
  }

  void Vsetvl(const typename Decoder::VsetvlArgs&) {
    Skip();
  }

  void Vsetvli(const typename Decoder::VsetvliArgs&) {
    Skip();
  }

  void Store(const typename Decoder::StoreArgs&) {
    Skip();
  }

  void Store(const typename Decoder::StoreFpArgs&) {
    Skip();
  }

  void System(const typename Decoder::SystemArgs&) {
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
    assembler_.Ld(Assembler::t3, addresses_[address]);
    assembler_.Jr(Assembler::t3);
  }

  void EmitAddresses() {
    for (auto& p : addresses_) {
      assembler_.Bind(&p.second);
      assembler_.Emit64(p.first);
    }
  }
};

size_t InstructionRelocator::Relocate(const void* address, int size, void** relocated) {
  return RV64Relocator::Relocate(static_cast<const uint16_t*>(address), size, relocated);
}

}  // namespace rv64hook
