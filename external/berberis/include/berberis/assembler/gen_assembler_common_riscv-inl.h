void Add(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'0033}>(arg0, arg1, arg2);
}
void Addi(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'0013}>(arg0, arg1, arg2);
}
void And(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'7033}>(arg0, arg1, arg2);
}
void Andi(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'7013}>(arg0, arg1, arg2);
}
void Auipc(Register arg0, UImmediate arg1) {
  EmitUTypeInstruction<uint32_t{0x0000'0017}>(arg0, arg1);
}
void Bcc(Condition arg0, Register arg1, Register arg2, BImmediate arg3);
void Bcc(Condition arg0, Register arg1, Register arg2, const Label& arg3);
void Beq(Register arg0, Register arg1, const Label& arg2);
void Beq(Register arg0, Register arg1, BImmediate arg2) {
  EmitBTypeInstruction<uint32_t{0x0000'0063}>(arg0, arg1, arg2);
}
void Beqz(Register arg0, const Label& arg1);
void Bge(Register arg0, Register arg1, const Label& arg2);
void Bge(Register arg0, Register arg1, BImmediate arg2) {
  EmitBTypeInstruction<uint32_t{0x0000'5063}>(arg0, arg1, arg2);
}
void Bgeu(Register arg0, Register arg1, const Label& arg2);
void Bgeu(Register arg0, Register arg1, BImmediate arg2) {
  EmitBTypeInstruction<uint32_t{0x0000'7063}>(arg0, arg1, arg2);
}
void Bgez(Register arg0, const Label& arg1);
void Bgt(Register arg0, Register arg1, const Label& arg2);
void Bgtu(Register arg0, Register arg1, const Label& arg2);
void Bgtz(Register arg0, const Label& arg1);
void Ble(Register arg0, Register arg1, const Label& arg2);
void Bleu(Register arg0, Register arg1, const Label& arg2);
void Blez(Register arg0, const Label& arg1);
void Blt(Register arg0, Register arg1, const Label& arg2);
void Blt(Register arg0, Register arg1, BImmediate arg2) {
  EmitBTypeInstruction<uint32_t{0x0000'4063}>(arg0, arg1, arg2);
}
void Bltu(Register arg0, Register arg1, const Label& arg2);
void Bltu(Register arg0, Register arg1, BImmediate arg2) {
  EmitBTypeInstruction<uint32_t{0x0000'6063}>(arg0, arg1, arg2);
}
void Bltz(Register arg0, const Label& arg1);
void Bne(Register arg0, Register arg1, const Label& arg2);
void Bne(Register arg0, Register arg1, BImmediate arg2) {
  EmitBTypeInstruction<uint32_t{0x0000'1063}>(arg0, arg1, arg2);
}
void Bnez(Register arg0, const Label& arg1);
void Call(const Label& arg0);
void Csrrc(Register arg0, Csr arg1, CsrImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'7073}>(arg0, arg1, arg2);
}
void Csrrc(Register arg0, Csr arg1, Register arg2) {
  EmitITypeInstruction<uint32_t{0x0000'3073}>(arg0, arg1, arg2);
}
void Csrrci(Register arg0, Csr arg1, CsrImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'7073}>(arg0, arg1, arg2);
}
void Csrrs(Register arg0, Csr arg1, CsrImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'6073}>(arg0, arg1, arg2);
}
void Csrrs(Register arg0, Csr arg1, Register arg2) {
  EmitITypeInstruction<uint32_t{0x0000'2073}>(arg0, arg1, arg2);
}
void Csrrsi(Register arg0, Csr arg1, CsrImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'6073}>(arg0, arg1, arg2);
}
void Csrrw(Register arg0, Csr arg1, CsrImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'5073}>(arg0, arg1, arg2);
}
void Csrrw(Register arg0, Csr arg1, Register arg2) {
  EmitITypeInstruction<uint32_t{0x0000'1073}>(arg0, arg1, arg2);
}
void Csrrwi(Register arg0, Csr arg1, CsrImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'5073}>(arg0, arg1, arg2);
}
void Div(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'4033}>(arg0, arg1, arg2);
}
void Divu(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'5033}>(arg0, arg1, arg2);
}
void FcvtDS(FpRegister arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0x4200'0053}>(arg0, arg1, arg2);
}

void FcvtDS(FpRegister arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0x4200'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtDW(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd200'0053}>(arg0, arg1, arg2);
}

void FcvtDW(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd200'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtDWu(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd210'0053}>(arg0, arg1, arg2);
}

void FcvtDWu(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd210'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtSD(FpRegister arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0x4010'0053}>(arg0, arg1, arg2);
}

void FcvtSD(FpRegister arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0x4010'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtSW(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd000'0053}>(arg0, arg1, arg2);
}

void FcvtSW(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd000'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtSWu(FpRegister arg0, Register arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xd010'0053}>(arg0, arg1, arg2);
}

void FcvtSWu(FpRegister arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0xd010'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtWD(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc200'0053}>(arg0, arg1, arg2);
}

void FcvtWD(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc200'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtWS(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc000'0053}>(arg0, arg1, arg2);
}

void FcvtWS(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc000'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtWuD(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc210'0053}>(arg0, arg1, arg2);
}

void FcvtWuD(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc210'0053}>(arg0, arg1, Rounding::kDyn);
}

void FcvtWuS(Register arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0xc010'0053}>(arg0, arg1, arg2);
}

void FcvtWuS(Register arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0xc010'0053}>(arg0, arg1, Rounding::kDyn);
}

void Fld(FpRegister arg0, const Label& arg1, Register arg2);
void Fld(FpRegister arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'3007}>(arg0, arg1);
}
void Flw(FpRegister arg0, const Label& arg1, Register arg2);
void Flw(FpRegister arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'2007}>(arg0, arg1);
}
void Fsd(FpRegister arg0, const Label& arg1, Register arg2);
void Fsd(FpRegister arg0, const Operand<Register, SImmediate>& arg1) {
  EmitSTypeInstruction<uint32_t{0x0000'3027}>(arg0, arg1);
}
void FsqrtD(FpRegister arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0x5a00'0053}>(arg0, arg1, arg2);
}

void FsqrtD(FpRegister arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0x5a00'0053}>(arg0, arg1, Rounding::kDyn);
}

void FsqrtS(FpRegister arg0, FpRegister arg1, Rounding arg2) {
  EmitRTypeInstruction<uint32_t{0x5800'0053}>(arg0, arg1, arg2);
}

void FsqrtS(FpRegister arg0, FpRegister arg1) {
  EmitRTypeInstruction<uint32_t{0x5800'0053}>(arg0, arg1, Rounding::kDyn);
}

void Fsw(FpRegister arg0, const Label& arg1, Register arg2);
void Fsw(FpRegister arg0, const Operand<Register, SImmediate>& arg1) {
  EmitSTypeInstruction<uint32_t{0x0000'2027}>(arg0, arg1);
}
void J(JImmediate arg0);
void Jal(JImmediate arg0);
void Jal(Register arg0, JImmediate arg1) {
  EmitJTypeInstruction<uint32_t{0x0000'006f}>(arg0, arg1);
}
void Jal(Register arg0, const Label& arg1);
void Jalr(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'0067}>(arg0, arg1, arg2);
}
void Jalr(Register arg0);
void Jalr(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'0067}>(arg0, arg1);
}
void Jr(Register arg0);
void La(Register arg0, const Label& arg1);
void Lb(Register arg0, const Label& arg1);
void Lb(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'0003}>(arg0, arg1);
}
void Lbu(Register arg0, const Label& arg1);
void Lbu(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'4003}>(arg0, arg1);
}
void Lh(Register arg0, const Label& arg1);
void Lh(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'1003}>(arg0, arg1);
}
void Lhu(Register arg0, const Label& arg1);
void Lhu(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'5003}>(arg0, arg1);
}
void Li(Register arg0, int32_t arg1);
template<typename ImmType>
auto Li(Register arg0, ImmType arg1) -> std::enable_if_t<std::is_integral_v<ImmType> && sizeof(int32_t) < sizeof(ImmType)> = delete;
void Lui(Register arg0, UImmediate arg1) {
  EmitUTypeInstruction<uint32_t{0x0000'0037}>(arg0, arg1);
}
void Lw(Register arg0, const Label& arg1);
void Lw(Register arg0, const Operand<Register, IImmediate>& arg1) {
  EmitITypeInstruction<uint32_t{0x0000'2003}>(arg0, arg1);
}
void Mul(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'0033}>(arg0, arg1, arg2);
}
void Mulh(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'1033}>(arg0, arg1, arg2);
}
void Mulhsu(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'2033}>(arg0, arg1, arg2);
}
void Mulhu(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'3033}>(arg0, arg1, arg2);
}
void Mv(Register arg0, Register arg1);
void Neg(Register arg0, Register arg1);
void Not(Register arg0, Register arg1);
void Or(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'6033}>(arg0, arg1, arg2);
}
void Ori(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'6013}>(arg0, arg1, arg2);
}
void PrefetchI(const Operand<Register, PImmediate>& arg0) {
  EmitPTypeInstruction<uint32_t{0x0000'6013}>(arg0);
}
void PrefetchR(const Operand<Register, PImmediate>& arg0) {
  EmitPTypeInstruction<uint32_t{0x0010'6013}>(arg0);
}
void PrefetchW(const Operand<Register, PImmediate>& arg0) {
  EmitPTypeInstruction<uint32_t{0x0030'6013}>(arg0);
}
void Rem(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'6033}>(arg0, arg1, arg2);
}
void Remu(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0200'7033}>(arg0, arg1, arg2);
}
void Ret();
void Ror(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x6000'5033}>(arg0, arg1, arg2);
}
void Sb(Register arg0, const Label& arg1, Register arg2);
void Sb(Register arg0, const Operand<Register, SImmediate>& arg1) {
  EmitSTypeInstruction<uint32_t{0x0000'0023}>(arg0, arg1);
}
void Seqz(Register arg0, Register arg1);
void SextB(Register arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0x6040'1013}>(arg0, arg1);
}
void SextH(Register arg0, Register arg1) {
  EmitRTypeInstruction<uint32_t{0x6050'1013}>(arg0, arg1);
}
void Sgtz(Register arg0, Register arg1);
void Sh(Register arg0, const Label& arg1, Register arg2);
void Sh(Register arg0, const Operand<Register, SImmediate>& arg1) {
  EmitSTypeInstruction<uint32_t{0x0000'1023}>(arg0, arg1);
}
void Sh3add(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x2000'6033}>(arg0, arg1, arg2);
}
void Sll(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'1033}>(arg0, arg1, arg2);
}
void Slt(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'2033}>(arg0, arg1, arg2);
}
void Slti(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'2013}>(arg0, arg1, arg2);
}
void Sltiu(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'3013}>(arg0, arg1, arg2);
}
void Sltu(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'3033}>(arg0, arg1, arg2);
}
void Sltz(Register arg0, Register arg1);
void Snez(Register arg0, Register arg1);
void Sra(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x4000'5033}>(arg0, arg1, arg2);
}
void Sraw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x4000'503b}>(arg0, arg1, arg2);
}
void Srl(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'5033}>(arg0, arg1, arg2);
}
void Srlw(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'503b}>(arg0, arg1, arg2);
}
void Sub(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x4000'0033}>(arg0, arg1, arg2);
}
void Sw(Register arg0, const Label& arg1, Register arg2);
void Sw(Register arg0, const Operand<Register, SImmediate>& arg1) {
  EmitSTypeInstruction<uint32_t{0x0000'2023}>(arg0, arg1);
}
void Tail(const Label& arg0);
void Xor(Register arg0, Register arg1, Register arg2) {
  EmitRTypeInstruction<uint32_t{0x0000'4033}>(arg0, arg1, arg2);
}
void Xori(Register arg0, Register arg1, IImmediate arg2) {
  EmitITypeInstruction<uint32_t{0x0000'4013}>(arg0, arg1, arg2);
}
