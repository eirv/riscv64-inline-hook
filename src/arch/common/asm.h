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

#ifndef __RV64HOOK_ARCH_COMMON_ASM_H__
#define __RV64HOOK_ARCH_COMMON_ASM_H__

#include "../../config.h"
#include "handle_offset.h"

#ifdef RV64HOOK_BUILD_TRAMPOLINE
#define ASM_LABEL(name) __rv64hook_##name
#else
#define ASM_LABEL(name) name
#endif

#define ASM_DEF(t, name)     \
  .global ASM_LABEL(name);   \
  .type ASM_LABEL(name), @t; \
  ASM_LABEL(name) :

#define ASM_FUNCTION(name) ASM_DEF(function, name)
#define ASM_FUNCTION_HIDDEN(name) \
  .hidden ASM_LABEL(name);        \
  ASM_FUNCTION(name)

#define ASM_OBJECT(name) ASM_DEF(object, name)
#define ASM_OBJECT_HIDDEN(name) \
  .hidden ASM_LABEL(name);      \
  ASM_OBJECT(name)

#define ASM_END(name) .size ASM_LABEL(name), .- ASM_LABEL(name)

#endif
