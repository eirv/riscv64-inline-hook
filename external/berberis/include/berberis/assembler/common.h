/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef BERBERIS_ASSEMBLER_COMMON_H_
#define BERBERIS_ASSEMBLER_COMMON_H_

#include <stdint.h>

#include <string>
#include <utility>

#include "berberis/assembler/machine_code.h"
#include "berberis/base/arena_alloc.h"
#include "berberis/base/arena_vector.h"
#include "berberis/base/logging.h"
#include "berberis/base/macros.h"  // DISALLOW_IMPLICIT_CONSTRUCTORS

namespace berberis {

class AssemblerBase {
 public:
  explicit AssemblerBase(MachineCode* code) : jumps_(code->arena()), code_(code) {}

  ~AssemblerBase() {}

  class Label {
   public:
    Label() : position_(kInvalid) {}

    // Label position is offset from the start of MachineCode
    // where it is bound to.
    uint32_t position() const { return position_; }

    void Bind(uint32_t position) {
      CHECK(!IsBound());
      position_ = position;
    }

    bool IsBound() const { return position_ != kInvalid; }

   protected:
    static const uint32_t kInvalid = 0xffffffff;
    uint32_t position_;

   private:
    DISALLOW_COPY_AND_ASSIGN(Label);
  };

  uint32_t pc() const { return code_->code_offset(); }

  // GNU-assembler inspired names: https://sourceware.org/binutils/docs-2.42/as.html#g_t8byte
  template <typename... Args>
  void Byte(Args... args) {
    static_assert((std::is_same_v<Args, uint8_t> && ...));
    (Emit8(args), ...);
  }

  template <typename... Args>
  void TwoByte(Args... args) {
    static_assert((std::is_same_v<Args, uint16_t> && ...));
    (Emit16(args), ...);
  }

  template <typename... Args>
  void FourByte(Args... args) {
    static_assert((std::is_same_v<Args, uint32_t> && ...));
    (Emit32(args), ...);
  }

  template <typename... Args>
  void EigthByte(Args... args) {
    static_assert((std::is_same_v<Args, uint64_t> && ...));
    (Emit64(args), ...);
  }

  // Macro operations.
  void Emit8(uint8_t v) { code_->AddU8(v); }

  void Emit16(int16_t v) { code_->Add<int16_t>(v); }

  void Emit32(int32_t v) { code_->Add<int32_t>(v); }

  void Emit64(int64_t v) { code_->Add<int64_t>(v); }

  template <typename T>
  void EmitSequence(const T* v, uint32_t count) {
    code_->AddSequence(v, sizeof(T) * count);
  }

  void Bind(Label* label) { label->Bind(pc()); }

  Label* MakeLabel() { return NewInArena<Label>(code_->arena()); }

  void SetRecoveryPoint(Label* recovery_label) {
    jumps_.push_back(Jump{recovery_label, pc(), true});
  }

 protected:
  template <typename T>
  T* AddrAs(uint32_t offset) {
    return code_->AddrAs<T>(offset);
  }

  void AddRelocation(uint32_t dst, RelocationType type, uint32_t pc, intptr_t data) {
    code_->AddRelocation(dst, type, pc, data);
  }

  // These are 'static' relocations, resolved when code is finalized.
  // We also have 'dynamic' relocations, resolved when code is installed.
  // TODO(b/232598137): rename Jump to something more appropriate since we are supporting
  // memory-accessing instructions, not just jumps.
  struct Jump {
    const Label* label;
    // Position of field to store offset.  Note: unless it's recovery label precomputed
    // "distance from the end of instruction" is stored there.
    //
    // This is needed because we keep pointer to the rip-offset field while value stored
    // there is counted from the end of instruction (on x86) or, sometimes, from the end
    // of next instruction (ARM).
    uint32_t pc;
    bool is_recovery;
  };
  using JumpList = ArenaVector<Jump>;
  JumpList jumps_;

 private:
  MachineCode* code_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AssemblerBase);
};

// Return the reverse condition. On all architectures that we may care about (AArch32/AArch64,
// RISC-V and x86) this can be achieved with a simple bitflop of the lowest bit.
// We may need a specialization of that function for more exotic architectures.
template <typename Condition>
inline constexpr Condition ToReverseCond(Condition cond) {
  CHECK(cond != Condition::kInvalidCondition);
  // Condition has a nice property that given a condition, you can get
  // its reverse condition by flipping the least significant bit.
  return Condition(static_cast<int>(cond) ^ 1);
}

}  // namespace berberis

#endif  // BERBERIS_ASSEMBLER_COMMON_H_
