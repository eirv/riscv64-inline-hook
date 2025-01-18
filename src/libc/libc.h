#pragma once

#include <cstdint>

extern "C" {
void* rv64hook_libc_memcpy(void *dest, const void *src, size_t count);
}
