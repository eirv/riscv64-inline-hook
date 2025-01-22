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

# 如何编译此文件?
#   1. 安装 Android NDK r27+, 并确保其可用
#   2. 打开终端, Windows 可以使用 git-bash
#   3. 输入: riscv64-linux-android35-clang -o trampoline.o -shared -nostdlib -nostartfiles trampoline.s
#   4. 输入: od -An -w2 -tx2 -v -j 65536 -N $(llvm-readelf --dyn-syms --elf-output-style=JSON trampoline.o | sed 's/.*\"Name\":\"trampoline\",\"Value\":1\},\"Value\":6553.,\"Size\":\([0-9]*\),.*/\1/') trampoline.o | awk "{for(i=1;i<=NF;++i)printf \"0x%s,\",\$i}"
#   5. 将打印的内容替换 trampoline.h 中的 kTrampoline

HookHandle_next = 8 * 3
HookHandle_pre_handler = 8 * 5
HookHandle_post_handler = 8 * 6
HookHandle_data = 8 * 7
HookHandle_enabled = 8 * 9

kRiscVVectorExtension = 1

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

.macro sregs save_t3=0
    sd      sp, -(8 * 64)(sp)
    addi    sp, sp, -(8 * 66)
    sd      ra, (8 * 1)(sp)
    sdt     gp,  tp,  t0,   t1,   8 * 3,  sp
    sdt     t2,  s0,  fp,   s1,   8 * 7,  sp
    sdt     a0,  a1,  a2,   a3,   8 * 11, sp
    sdt     a4,  a5,  a6,   a7,   8 * 15, sp
    sdt     s2,  s3,  s4,   s5,   8 * 19, sp
    sdt     s6,  s7,  s8,   s9,   8 * 23, sp

    .if \save_t3 != 0
        sdt s10, s11, t3,   t4,   8 * 27, sp
    .else
        sdp s10, s11,             8 * 27, sp
        sd  t4,                  (8 * 30)(sp)
    .endif

    sdp     t5,  t6,              8 * 31, sp
    fsdt    ft0, ft1, ft2,  ft3,  8 * 33, sp
    fsdt    ft4, ft5, ft6,  ft7,  8 * 37, sp
    fsdt    fs0, fs1, fa0,  fa1,  8 * 41, sp
    fsdt    fa2, fa3, fa4,  fa5,  8 * 45, sp
    fsdt    fa6, fa7, fs2,  fs3,  8 * 49, sp
    fsdt    fs4, fs5, fs6,  fs7,  8 * 53, sp
    fsdt    fs8, fs9, fs10, fs11, 8 * 57, sp
    fsdt    ft8, ft9, ft10, ft11, 8 * 61, sp
.endm

.macro pregs pop_t3=0
    fldt    ft8, ft9, ft10, ft11, 8 * 61, sp
    fldt    fs8, fs9, fs10, fs11, 8 * 57, sp
    fldt    fs4, fs5, fs6,  fs7,  8 * 53, sp
    fldt    fa6, fa7, fs2,  fs3,  8 * 49, sp
    fldt    fa2, fa3, fa4,  fa5,  8 * 45, sp
    fldt    fs0, fs1, fa0,  fa1,  8 * 41, sp
    fldt    ft4, ft5, ft6,  ft7,  8 * 37, sp
    fldt    ft0, ft1, ft2,  ft3,  8 * 33, sp
    ldp     t5,  t6,              8 * 31, sp

    .if \pop_t3 != 0
        ldt s10, s11, t3,   t4,   8 * 27, sp
    .else
        ld  t4,                  (8 * 30)(sp)
        ldp s10, s11,             8 * 27, sp
    .endif

    ldt     s6,  s7,  s8,   s9,   8 * 23, sp
    ldt     s2,  s3,  s4,   s5,   8 * 19, sp
    ldt     a4,  a5,  a6,   a7,   8 * 15, sp
    ldt     a0,  a1,  a2,   a3,   8 * 11, sp
    ldt     t2,  s0,  fp,   s1,   8 * 7,  sp
    ldt     gp,  tp,  t0,   t1,   8 * 3,  sp
    ld      ra, (8 * 1)(sp)
    ld      sp, (8 * 2)(sp)
.endm

.macro callrh off, handle
    ld      a1, \handle
1:
    ld      t3, HookHandle_next(a1)
    sd      t3, 0(sp)
    lbu     t3, HookHandle_enabled(a1)
    beqz    t3, 2f
    ld      t3, \off(a1)
    beqz    t3, 2f
    addi    a0, sp, 8
    ld      a2, HookHandle_data(a1)
    jalr    t3
2:
    ld      a1, 0(sp)
    bnez    a1, 1b
.endm

    .text
    .balign 64 * 1024
    .global trampoline
    .type   trampoline, @function

trampoline:
    ld      t3, .L.hook
    beqz    t3, .L.call_register_handlers
    jr      t3

.L.jump_backup:
    ld      t3, .L.backup
    jr      t3

.L.call_register_handlers:
    sregs
    sd      zero, (8 * 65)(sp)
    callrh  HookHandle_pre_handler, .L.root_handle

    .if kRiscVVectorExtension
        vsetivli zero, 1, e64, m1, ta, ma
        vmv.v.i  v28, 0
        addi     t3, sp, 8 * 65
        vle8.v   v28, (t3)
    .endif

    pregs

    .if kRiscVVectorExtension
        vmv.x.s t3, v28
    .else
        lbu     t3, -(8 * 1)(sp)
    .endif

    bnez    t3, .L.return
    lhu     t3, .L.post_handlers
    beqz    t3, .L.jump_backup

    .if kRiscVVectorExtension
        vmv.v.x v28, ra
    .else
        fmv.d.x ft11, ra
    .endif

    ld      t3, .L.backup
    jalr    t3

    .if kRiscVVectorExtension
        vmv.x.s ra, v28
    .else
        fmv.x.d ra, ft11
    .endif

    sregs   1
    callrh  HookHandle_post_handler, .L.root_handle
    pregs   1

.L.return:
    ret

    .size   trampoline, . - trampoline

data:
.L.root_handle:
    .quad   0x1122334455667788
.L.hook:
    .quad   0x1122334455667788
.L.backup:
    .quad   0x1122334455667788
.L.post_handlers:
    .hword  0x1234
