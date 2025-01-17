void AddUW(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0800'003b}>(arg0, arg1, arg2);
}
void Addiw(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'001b}>(arg0, arg1, arg2);
}
void Addw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'003b}>(arg0, arg1, arg2);
}
void Bexti(Register arg0, Register arg1, Shift64Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x4800'5013}>(arg0, arg1, arg2);
}
void Divuw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'503b}>(arg0, arg1, arg2);
}
void Divw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'403b}>(arg0, arg1, arg2);
}
void FcvtDL(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd220'0053}>(arg0, arg1, arg2);
}

void FcvtDL(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd220'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtDLu(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd230'0053}>(arg0, arg1, arg2);
}

void FcvtDLu(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd230'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtLD(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc220'0053}>(arg0, arg1, arg2);
}

void FcvtLD(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc220'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtLS(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc020'0053}>(arg0, arg1, arg2);
}

void FcvtLS(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc020'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtLuD(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc230'0053}>(arg0, arg1, arg2);
}

void FcvtLuD(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc230'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtLuS(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc030'0053}>(arg0, arg1, arg2);
}

void FcvtLuS(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc030'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtSL(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd020'0053}>(arg0, arg1, arg2);
}

void FcvtSL(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd020'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtSLu(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd030'0053}>(arg0, arg1, arg2);
}

void FcvtSLu(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd030'0053}>(arg0, arg1, Rounding::kDyn);
}

void Ld(Register arg0, const Label& arg1);
void Ld(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'3003}>(arg0, arg1);
}
void Li(Register arg0, int64_t arg1);
template<typename ImmType>
auto Li(Register arg0, ImmType arg1) -> std::enable_if_t<std::is_integral_v<ImmType> && sizeof(int64_t) < sizeof(ImmType)> = delete;
void Lwu(Register arg0, const Label& arg1);
void Lwu(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'6003}>(arg0, arg1);
}
void Mulw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'003b}>(arg0, arg1, arg2);
}
void Negw(Register arg0, Register arg1);
void Remuw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'703b}>(arg0, arg1, arg2);
}
void Remw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'603b}>(arg0, arg1, arg2);
}
void Rori(Register arg0, Register arg1, Shift64Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x6000'5013}>(arg0, arg1, arg2);
}
void Roriw(Register arg0, Register arg1, Shift32Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x6000'501b}>(arg0, arg1, arg2);
}
void Rorw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x6000'503b}>(arg0, arg1, arg2);
}
void Sd(Register arg0, const Label& arg1, Register arg2);
void Sd(Register arg0, const Operand<Register, SImmediate>& arg1) {
  EmitSTypeInstruction<uint32_t{0x0000'3023}>(arg0, arg1);
}
void SextW(Register arg0, Register arg1);
void Slli(Register arg0, Register arg1, Shift64Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'1013}>(arg0, arg1, arg2);
}
void Slliw(Register arg0, Register arg1, Shift32Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'101b}>(arg0, arg1, arg2);
}
void Sllw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'103b}>(arg0, arg1, arg2);
}
void Srai(Register arg0, Register arg1, Shift64Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x4000'5013}>(arg0, arg1, arg2);
}
void Sraiw(Register arg0, Register arg1, Shift32Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x4000'501b}>(arg0, arg1, arg2);
}
void Srli(Register arg0, Register arg1, Shift64Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'5013}>(arg0, arg1, arg2);
}
void Srliw(Register arg0, Register arg1, Shift32Immediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'501b}>(arg0, arg1, arg2);
}
void Subw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x4000'003b}>(arg0, arg1, arg2);
}
void ZextW(Register arg0, Register arg1);
