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

#ifndef __RV64HOOK_ARCH_COMMON_HANDLE_OFFSET_H__
#define __RV64HOOK_ARCH_COMMON_HANDLE_OFFSET_H__

#define HookHandle_next         (8 * 4)
#define HookHandle_pre_handler  (8 * 6)
#define HookHandle_post_handler (8 * 7)
#define HookHandle_data         (8 * 8)
#define HookHandle_enabled      (8 * 10)

#endif
