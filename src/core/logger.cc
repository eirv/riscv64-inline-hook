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

#include "logger.h"

#include <cstdio>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace rv64hook {

static char error_buf[128];
static bool has_error = false;

void ClearError() {
  has_error = false;
}

void SetError(const char* tag, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  char tmp[sizeof(error_buf)];
  snprintf(tmp, sizeof(tmp), "[%s] %s", tag, fmt);
  vsnprintf(error_buf, sizeof(error_buf), tmp, va);
  has_error = true;
  va_end(va);
}

[[gnu::visibility("default"), maybe_unused]] const char* GetLastError() {
  if (has_error) {
    has_error = false;
    return error_buf;
  }
  return nullptr;
}

}  // namespace rv64hook
