/*
 * Copyright (c) 2023 SiFive, Inc. -- Proprietary and Confidential All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * SiFive, Inc. The intellectual and technical concepts contained herein are
 * proprietary to SiFive, Inc. and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 *
 * This work may not be copied, modified, re-published, uploaded, executed, or
 * distributed in any way, in any medium, whether in whole or in part, without
 * prior written permission from SiFive, Inc.
 *
 * The copyright notice above does not evidence any actual or intended publication
 * or disclosure of this source code, which includes information that is
 * confidential and/or proprietary, and is a trade secret, of SiFive, Inc.
 *
 */
#ifndef _SYS_ASM_H
#define _SYS_ASM_H

#undef LEAF
#define LEAF(symbol)       \
  .globl symbol;           \
  .hidden symbol;          \
  .align 2;                \
  .type symbol, @function; \
  symbol:                  \
  .cfi_startproc;

#undef END
#define END(function) \
  .cfi_endproc;       \
  .size function, .- function

#define ENTRY(name) LEAF(name)

#define L(label) .L##label

#endif /* _SYS_ASM_H */
