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

#include "../common/asm.h"

.macro sregs
    sub     sp,  sp,  #8 * 98

    stp     x0,  x1,  [sp]
    stp     x2,  x3,  [sp, #8 * 2]
    stp     x4,  x5,  [sp, #8 * 4]
    stp     x6,  x7,  [sp, #8 * 6]
    stp     x8,  x9,  [sp, #8 * 8]
    stp     x10, x11, [sp, #8 * 10]
    stp     x12, x13, [sp, #8 * 12]
    stp     x14, x15, [sp, #8 * 14]
    stp     x16, x17, [sp, #8 * 16]
    stp     x18, x19, [sp, #8 * 18]
    stp     x20, x21, [sp, #8 * 20]
    stp     x22, x23, [sp, #8 * 22]
    stp     x24, x25, [sp, #8 * 24]
    stp     x26, x27, [sp, #8 * 26]
    stp     x28, fp,  [sp, #8 * 28]

    add     x16, sp,  #8 * 98
    stp     lr,  x16, [sp, #8 * 30]
    add     fp,  sp,  #8 * 29

    stp     q0,  q1,  [sp, #8 * 32]
    stp     q2,  q3,  [sp, #8 * 36]
    stp     q4,  q5,  [sp, #8 * 40]
    stp     q6,  q7,  [sp, #8 * 44]

    .if FULL_FLOATING_POINT_REGISTER_PACK
    stp     q8,  q9,  [sp, #8 * 48]
    stp     q10, q11, [sp, #8 * 52]
    stp     q12, q13, [sp, #8 * 56]
    stp     q14, q15, [sp, #8 * 60]
    stp     q16, q17, [sp, #8 * 64]
    stp     q18, q19, [sp, #8 * 68]
    stp     q20, q21, [sp, #8 * 72]
    stp     q22, q23, [sp, #8 * 76]
    stp     q24, q25, [sp, #8 * 80]
    stp     q26, q27, [sp, #8 * 84]
    stp     q28, q29, [sp, #8 * 88]
    stp     q30, q31, [sp, #8 * 92]
    .endif
.endm

.macro pregs
    .if FULL_FLOATING_POINT_REGISTER_PACK
    ldp     q30, q31, [sp, #8 * 92]
    ldp     q28, q29, [sp, #8 * 88]
    ldp     q26, q27, [sp, #8 * 84]
    ldp     q24, q25, [sp, #8 * 80]
    ldp     q22, q23, [sp, #8 * 76]
    ldp     q20, q21, [sp, #8 * 72]
    ldp     q18, q19, [sp, #8 * 68]
    ldp     q16, q17, [sp, #8 * 64]
    ldp     q14, q15, [sp, #8 * 60]
    ldp     q12, q13, [sp, #8 * 56]
    ldp     q10, q11, [sp, #8 * 52]
    ldp     q8,  q9,  [sp, #8 * 48]
    .endif

    ldp     q6,  q7,  [sp, #8 * 44]
    ldp     q4,  q5,  [sp, #8 * 40]
    ldp     q2,  q3,  [sp, #8 * 36]
    ldp     q0,  q1,  [sp, #8 * 32]

    ldr     lr,       [sp, #8 * 30]
    ldp     x28, fp,  [sp, #8 * 28]
    ldp     x26, x27, [sp, #8 * 26]
    ldp     x24, x25, [sp, #8 * 24]
    ldp     x22, x23, [sp, #8 * 22]
    ldp     x20, x21, [sp, #8 * 20]
    ldp     x18, x19, [sp, #8 * 18]
    ldp     x16, x17, [sp, #8 * 16]
    ldp     x14, x15, [sp, #8 * 14]
    ldp     x12, x13, [sp, #8 * 12]
    ldp     x10, x11, [sp, #8 * 10]
    ldp     x8,  x9,  [sp, #8 * 8]
    ldp     x6,  x7,  [sp, #8 * 6]
    ldp     x4,  x5,  [sp, #8 * 4]
    ldp     x2,  x3,  [sp, #8 * 2]
    ldp     x0,  x1,  [sp]

    add     sp,  sp,  #8 * 98
.endm

.macro callrh off, handle
    ldr     x1, \handle
1:
    ldr     TMP_GENERIC_REGISTER, [x1, #HookHandle_next]
    str     TMP_GENERIC_REGISTER, [sp, #8 * 97]
    ldrb    TMP_GENERIC_REGISTER_32, [x1, #HookHandle_enabled]
    cbz     TMP_GENERIC_REGISTER_32, 2f
    ldr     TMP_GENERIC_REGISTER, [x1, #\off]
    cbz     TMP_GENERIC_REGISTER, 2f
    mov     x0, sp
    ldr     x2, [x1, #HookHandle_data]
    blr     TMP_GENERIC_REGISTER
2:
    ldr     x1, [sp, #8 * 97]
    cbnz    x1, 1b
.endm

    .text
    .balign 64 * 1024
ASM_FUNCTION_HIDDEN(trampoline)
    adr     TMP_GENERIC_REGISTER, .L.data.enabled
    ldrb    TMP_GENERIC_REGISTER_32, [TMP_GENERIC_REGISTER]
    cbz     TMP_GENERIC_REGISTER_32, .L.jump_backup
    ldr     TMP_GENERIC_REGISTER, .L.data.hook
    cbz     TMP_GENERIC_REGISTER, .L.call_register_handlers
    br      TMP_GENERIC_REGISTER

.L.jump_backup:
    ldr     TMP_GENERIC_REGISTER, .L.data.backup
    br      TMP_GENERIC_REGISTER

.L.call_register_handlers:
    sregs
    str     xzr, [sp, #8 * 96]

    callrh  HookHandle_pre_handler, .L.data.root_handle

    ldr     TMP_GENERIC_REGISTER, .L.data.setspecific
    cbz     TMP_GENERIC_REGISTER, .L.store_lr_ext
    ldr     w0, .L.data.tls_key
    ldr     x1, [sp, #8 * 30]
    blr     TMP_GENERIC_REGISTER
    b       .L.pop_pre_registers

.L.store_lr_ext:
    .if USE_VECTOR_EXTENSION
    // TODO
    .else
    str     lr, [sp, #8 * 97]
    ldr     TMP_FLOAT_REGISTER, [sp, #8 * 97]
    .endif

.L.pop_pre_registers:
    ldrb    TMP_GENERIC_REGISTER_32, [sp, #8 * 96]
    cmp     TMP_GENERIC_REGISTER_32, #0
    pregs

    b.ne    .L.return
    adr     TMP_GENERIC_REGISTER, .L.data.post_handlers
    ldrh    TMP_GENERIC_REGISTER_32, [TMP_GENERIC_REGISTER]
    cbz     TMP_GENERIC_REGISTER_32, .L.jump_backup

    ldr     TMP_GENERIC_REGISTER, .L.data.backup
    blr     TMP_GENERIC_REGISTER

    sregs

    ldr     TMP_GENERIC_REGISTER, .L.data.getspecific
    cbz     TMP_GENERIC_REGISTER, .L.load_lr_ext
    ldr     w0, .L.data.tls_key
    blr     TMP_GENERIC_REGISTER
    str     x0, [sp, #8 * 30]
    b       .L.call_post_register_handlers

.L.load_lr_ext:
    .if USE_VECTOR_EXTENSION
    // TODO
    sd      TMP_GENERIC_REGISTER, (8 * 1)(sp)
    .else
    str     TMP_FLOAT_REGISTER, [sp, #8 * 97]
    ldr     lr, [sp, #8 * 97]
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
