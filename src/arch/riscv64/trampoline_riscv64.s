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

// 如何编译此文件?
//   1. 安装 Android NDK >= r27, 并确保其可用
//   2. 打开终端, Windows 可以使用 git-bash
//   3. $ riscv64-linux-android35-clang -o trampoline.o -shared -nostdlib -nostartfiles trampoline_riscv64.S
//   4. $ od -An -w2 -tx2 -v -j 65536 -N $(llvm-readelf --symbols --elf-output-style=JSON trampoline.o | sed 's/.*\"Name\":\"trampoline\",\"Value\":.\},\"Value\":6553.,\"Size\":\([0-9]*\),.*/\1/') trampoline.o | awk "{for(i=1;i<=NF;++i)printf \"0x%s,\",\$i}"
//   5. 将打印的内容替换 trampoline_riscv64.h 中的 kTrampoline

#include "../common/asm.h"

.macro sdp r1, r2, off, rs1
    sd      \r1, (\off)(\rs1)
    sd      \r2, (\off)+8(\rs1)
.endm

.macro sdt r1, r2, r3, r4, off, rs1
    sdp     \r1, \r2, \off, \rs1
    sdp     \r3, \r4, (\off)+16, \rs1
.endm

.macro ldp r1, r2, off, rs1
    ld      \r2, (\off)+8(\rs1)
    ld      \r1, (\off)(\rs1)
.endm

.macro ldt r1, r2, r3, r4, off, rs1
    ldp     \r3, \r4, (\off)+16, \rs1
    ldp     \r1, \r2, \off, \rs1
.endm

.macro fsdp r1, r2, off, rs1
    fsd     \r1, (\off)(\rs1)
    fsd     \r2, (\off)+8(\rs1)
.endm

.macro fsdt r1, r2, r3, r4, off, rs1
    fsdp    \r1, \r2, \off, \rs1
    fsdp    \r3, \r4, (\off)+16, \rs1
.endm

.macro fldp r1, r2, off, rs1
    fld     \r2, (\off)+8(\rs1)
    fld     \r1, (\off)(\rs1)
.endm

.macro fldt r1, r2, r3, r4, off, rs1
    fldp    \r3, \r4, (\off)+16, \rs1
    fldp    \r1, \r2, \off, \rs1
.endm

.macro sregs store_ra=0
    sd      sp,                 -(8 * 63)(sp)
    addi    sp,  sp,            -(8 * 65)
    .if \store_ra != 0
    sd      ra,                  (8 * 1)(sp)
    .endif
    sdt     gp,  tp,  t0,   t1,   8 * 3,  sp
    sdt     t2,  s0,  s1,   a0,   8 * 7,  sp
    sdt     a1,  a2,  a3,   a4,   8 * 11, sp
    sdt     a5,  a6,  a7,   s2,   8 * 15, sp
    sdt     s3,  s4,  s5,   s6,   8 * 19, sp
    sdt     s7,  s8,  s9,   s10,  8 * 23, sp
    sdt     s11, t3,  t4,   t5,   8 * 27, sp
    sd      t6,                  (8 * 31)(sp)
    .if FULL_FLOATING_POINT_REGISTER_PACK
    fsdt    ft0, ft1, ft2,  ft3,  8 * 32, sp
    fsdt    ft4, ft5, ft6,  ft7,  8 * 36, sp
    fsdt    fs0, fs1, fa0,  fa1,  8 * 40, sp
    fsdt    fa2, fa3, fa4,  fa5,  8 * 44, sp
    fsdt    fa6, fa7, fs2,  fs3,  8 * 48, sp
    fsdt    fs4, fs5, fs6,  fs7,  8 * 52, sp
    fsdt    fs8, fs9, fs10, fs11, 8 * 56, sp
    fsdt    ft8, ft9, ft10, ft11, 8 * 60, sp
    .else
    fsdt    fa0, fa1, fa2,  fa3,  8 * 42, sp
    fsdt    fa4, fa5, fa6,  fa7,  8 * 46, sp
    .endif
.endm

.macro pregs
    .if FULL_FLOATING_POINT_REGISTER_PACK
    fldt    ft8, ft9, ft10, ft11, 8 * 60, sp
    fldt    fs8, fs9, fs10, fs11, 8 * 56, sp
    fldt    fs4, fs5, fs6,  fs7,  8 * 52, sp
    fldt    fa6, fa7, fs2,  fs3,  8 * 48, sp
    fldt    fa2, fa3, fa4,  fa5,  8 * 44, sp
    fldt    fs0, fs1, fa0,  fa1,  8 * 40, sp
    fldt    ft4, ft5, ft6,  ft7,  8 * 36, sp
    fldt    ft0, ft1, ft2,  ft3,  8 * 32, sp
    .else
    fldt    fa4, fa5, fa6,  fa7,  8 * 46, sp
    fldt    fa0, fa1, fa2,  fa3,  8 * 42, sp
    .endif
    ld      t6,                  (8 * 31)(sp)
    ldt     s11, t3,  t4,   t5,   8 * 27, sp
    ldt     s7,  s8,  s9,  s10,   8 * 23, sp
    ldt     s3,  s4,  s5,  s6,    8 * 19, sp
    ldt     a5,  a6,  a7,  s2,    8 * 15, sp
    ldt     a1,  a2,  a3,  a4,    8 * 11, sp
    ldt     t2,  s0,  s1,  a0,    8 * 7,  sp
    ldt     gp,  tp,  t0,  t1,    8 * 3,  sp
    ld      ra,                  (8 * 1)(sp)
    ld      sp,                  (8 * 2)(sp)
.endm

.macro callrh off, handle
    ld      a1, \handle
1:
    ld      TMP_GENERIC_REGISTER, HookHandle_next(a1)
    sd      TMP_GENERIC_REGISTER, 0(sp)
    lb      TMP_GENERIC_REGISTER, HookHandle_enabled(a1)
    beqz    TMP_GENERIC_REGISTER, 2f
    ld      TMP_GENERIC_REGISTER, \off(a1)
    beqz    TMP_GENERIC_REGISTER, 2f
    addi    a0, sp, 8
    ld      a2, HookHandle_data(a1)
    jalr    TMP_GENERIC_REGISTER
2:
    ld      a1, 0(sp)
    bnez    a1, 1b
.endm

    .text
    .balign 64 * 1024
ASM_FUNCTION_HIDDEN(trampoline)
    lb      TMP_GENERIC_REGISTER, .L.data.enabled
    beqz    TMP_GENERIC_REGISTER, .L.jump_backup
    ld      TMP_GENERIC_REGISTER, .L.data.hook
    beqz    TMP_GENERIC_REGISTER, .L.call_register_handlers
    jr      TMP_GENERIC_REGISTER

.L.jump_backup:
    ld      TMP_GENERIC_REGISTER, .L.data.backup
    jr      TMP_GENERIC_REGISTER

.L.call_register_handlers:
    sregs   1
    sd      zero, (8 * 64)(sp)

    callrh  HookHandle_pre_handler, .L.data.root_handle

    ld      TMP_GENERIC_REGISTER, .L.data.setspecific
    beqz    TMP_GENERIC_REGISTER, .L.store_ra_ext
    lw      a0, .L.data.tls_key
    ld      a1, (8 * 1)(sp)
    jalr    TMP_GENERIC_REGISTER
    j       .L.pop_pre_registers

.L.store_ra_ext:
    .if USE_VECTOR_EXTENSION
    vsetivli zero, 1, e64, m1, ta, ma
    addi    TMP_GENERIC_REGISTER, sp, 8 * 1
    vle64.v TMP_VECTOR_REGISTER, (TMP_GENERIC_REGISTER)
    .else
    fmv.d.x TMP_FLOAT_REGISTER, ra
    .endif

.L.pop_pre_registers:
    lb      TMP_GENERIC_REGISTER, (8 * 64)(sp)
    pregs

    bnez    TMP_GENERIC_REGISTER, .L.return
    lh      TMP_GENERIC_REGISTER, .L.data.post_handlers
    beqz    TMP_GENERIC_REGISTER, .L.jump_backup

    ld      TMP_GENERIC_REGISTER, .L.data.backup
    jalr    TMP_GENERIC_REGISTER

    sregs

    ld      TMP_GENERIC_REGISTER, .L.data.getspecific
    beqz    TMP_GENERIC_REGISTER, .L.load_ra_ext
    lw      a0, .L.data.tls_key
    jalr    TMP_GENERIC_REGISTER
    sd      a0, (8 * 1)(sp)
    j       .L.call_post_register_handlers

.L.load_ra_ext:
    .if USE_VECTOR_EXTENSION
    vsetivli zero, 1, e64, m1, ta, ma
    vmv.x.s TMP_GENERIC_REGISTER, TMP_VECTOR_REGISTER
    sd      TMP_GENERIC_REGISTER, (8 * 1)(sp)
    .else
    fmv.x.d ra, TMP_FLOAT_REGISTER
    .endif

.L.call_post_register_handlers:
    callrh  HookHandle_post_handler, .L.data.root_handle

    pregs

.L.return:
    ret

ASM_FUNCTION_HIDDEN(trampoline_end)
ASM_END(trampoline)

ASM_OBJECT_HIDDEN(data)
.L.data.root_handle:
    .quad   0x1122334455667788
.L.data.hook:
    .quad   0x1122334455667788
.L.data.backup:
    .quad   0x1122334455667788
.L.data.getspecific:
    .quad   0x1122334455667788
.L.data.setspecific:
    .quad   0x1122334455667788
.L.data.tls_key:
    .int    0x12345678
.L.data.post_handlers:
    .hword  0x1234
.L.data.enabled:
    .byte   0x12
ASM_END(data)
