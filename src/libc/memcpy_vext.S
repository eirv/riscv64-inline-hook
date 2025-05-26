// Copyright (c) 2023 SiFive, Inc. -- Proprietary and Confidential All Rights
// Reserved.
//
// NOTICE: All information contained herein is, and remains the property of
// SiFive, Inc. The intellectual and technical concepts contained herein are
// proprietary to SiFive, Inc. and may be covered by U.S. and Foreign Patents,
// patents in process, and are protected by trade secret or copyright law.
//
// This work may not be copied, modified, re-published, uploaded, executed, or
// distributed in any way, in any medium, whether in whole or in part, without
// prior written permission from SiFive, Inc.
//
// The copyright notice above does not evidence any actual or intended
// publication or disclosure of this source code, which includes information
// that is confidential and/or proprietary, and is a trade secret, of SiFive,
// Inc.
//===----------------------------------------------------------------------===//

// Contributed by: Jerry Shih <jerry.shih@sifive.com>

// Prototype:
// void* memcpy(void *dest, const void *src, size_t count)

#include "config.h"

#define ENTRY(symbol)        \
    .globl symbol;           \
    .hidden symbol;          \
    .align 2;                \
    .type symbol, @function; \
    symbol:                  \
    .cfi_startproc;

#define END(function)   \
    .cfi_endproc;       \
    .size function, .- function

#define pDst a0
#define pSrc a1
#define iNum a2

#define iVL a3

#define ELEM_LMUL_SETTING m8
#define vData v0

#if USE_VECTOR_EXTENSION
ENTRY(__rv64hook_libc_memcpy_vext)

.L.loop:
    vsetvli iVL, iNum, e8, ELEM_LMUL_SETTING, ta, ma

    vle8.v  vData, (pSrc)
    sub     iNum, iNum, iVL
    add     pSrc, pSrc, iVL
    vse8.v  vData, (pDst)
    add     pDst, pDst, iVL

    bnez    iNum, .L.loop

    ret

END(__rv64hook_libc_memcpy_vext)
#endif
