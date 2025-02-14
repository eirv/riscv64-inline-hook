/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "libc.h"

namespace rv64hook {

static constexpr auto wsize = sizeof(intptr_t);
static constexpr auto wmask = wsize - 1;

void __rv64hook_libc_memcpy_gc(void* dst0, const void* src0, size_t length) {
  auto dst = reinterpret_cast<char*>(dst0);
  auto src = reinterpret_cast<const char*>(src0);
  size_t t;

  if (length == 0 || dst == src) [[unlikely]] {
    // nothing to do
    return;
  }

  // Macros: loop-t-times; and loop-t-times, t>0
#define TLOOP(s) \
  if (t) TLOOP1(s)
#define TLOOP1(s) \
  do {            \
    s;            \
  } while (--t)

  if (reinterpret_cast<uintptr_t>(dst) < reinterpret_cast<uintptr_t>(src)) {
    // Copy forward.
    t = reinterpret_cast<uintptr_t>(src);  // only need low bits
    if ((t | reinterpret_cast<uintptr_t>(dst)) & wmask) {
      // Try to align operands.
      // This cannot be done unless the low bits match.
      if ((t ^ reinterpret_cast<uintptr_t>(dst)) & wmask || length < wsize) t = length;
      else t = wsize - (t & wmask);
      length -= t;
      TLOOP1(*dst++ = *src++);
    }
    // Copy whole words, then mop up any trailing bytes.
    t = length / wsize;
    TLOOP(*reinterpret_cast<uintptr_t*>(dst) = *reinterpret_cast<const uintptr_t*>(src);
          src += wsize;
          dst += wsize);
    t = length & wmask;
    TLOOP(*dst++ = *src++);
  } else {
    // Copy backwards.
    // Otherwise essentially the same.
    // Alignment works as before, except that it takes (t&wmask) bytes to align,
    // not wsize-(t&wmask).
    src += length;
    dst += length;
    t = reinterpret_cast<uintptr_t>(src);
    if ((t | reinterpret_cast<uintptr_t>(dst)) & wmask) {
      if ((t ^ reinterpret_cast<uintptr_t>(dst)) & wmask || length <= wsize) t = length;
      else t &= wmask;
      length -= t;
      TLOOP1(*--dst = *--src);
    }
    t = length / wsize;
    TLOOP(src -= wsize; dst -= wsize;
          *reinterpret_cast<uintptr_t*>(dst) = *reinterpret_cast<const uintptr_t*>(src));
    t = length & wmask;
    TLOOP(*--dst = *--src);
  }
}

}  // namespace rv64hook
