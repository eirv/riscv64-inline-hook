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

// Assembler to produce RV64 instructions (UABI version). Somewhat influenced by V8 assembler.

#ifndef BERBERIS_ASSEMBLER_RV64I_H_
#define BERBERIS_ASSEMBLER_RV64I_H_

#include <type_traits>  // std::is_same

#include "berberis/assembler/rv64.h"

namespace berberis {

namespace rv64i {

class Assembler : public ::berberis::rv64::Assembler {
 public:
  using BaseAssembler = riscv::Assembler<::berberis::rv64::Assembler>;
  using FinalAssembler = ::berberis::rv64::Assembler;

  explicit Assembler(MachineCode* code) : berberis::rv64::Assembler(code) {}

  static constexpr Register ra{1};
  static constexpr Register sp{2};
  static constexpr Register gp{3};
  static constexpr Register tp{4};
  static constexpr Register t0{5};
  static constexpr Register t1{6};
  static constexpr Register t2{7};
  static constexpr Register s0{8};
  static constexpr Register fp{8};
  static constexpr Register s1{9};
  static constexpr Register a0{10};
  static constexpr Register a1{11};
  static constexpr Register a2{12};
  static constexpr Register a3{13};
  static constexpr Register a4{14};
  static constexpr Register a5{15};
  static constexpr Register a6{16};
  static constexpr Register a7{17};
  static constexpr Register s2{18};
  static constexpr Register s3{19};
  static constexpr Register s4{20};
  static constexpr Register s5{21};
  static constexpr Register s6{22};
  static constexpr Register s7{23};
  static constexpr Register s8{24};
  static constexpr Register s9{25};
  static constexpr Register s10{26};
  static constexpr Register s11{27};
  static constexpr Register t3{28};
  static constexpr Register t4{29};
  static constexpr Register t5{30};
  static constexpr Register t6{31};

  Assembler() = delete;
  Assembler(const Assembler&) = delete;
  Assembler(Assembler&&) = delete;
  void operator=(const Assembler&) = delete;
  void operator=(Assembler&&) = delete;
};

}  // namespace rv64i

}  // namespace berberis

#endif  // BERBERIS_ASSEMBLER_RV64I_H_
