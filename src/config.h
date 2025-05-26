#ifndef __RV64HOOK_CONFIG_H__
#define __RV64HOOK_CONFIG_H__

#if defined(__riscv)
#define TMP_GENERIC_REGISTER t3
#define TMP_FLOAT_REGISTER   ft11
#define TMP_VECTOR_REGISTER  v28
#define USE_VECTOR_EXTENSION IS_VECTOR_SUPPORTED
#elif defined(__aarch64__)
#define TMP_GENERIC_REGISTER    x16
#define TMP_GENERIC_REGISTER_32 w16
#define TMP_FLOAT_REGISTER      d16
#define TMP_VECTOR_REGISTER     v16
#define USE_VECTOR_EXTENSION    0
#endif
#define STORE_RETURN_ADDRESS_BY_TLS       1
#define FULL_FLOATING_POINT_REGISTER_PACK 1

#if defined(__riscv_vector) || __has_include(<arm_neon.h>)
#define IS_VECTOR_SUPPORTED 1
#else
#define IS_VECTOR_SUPPORTED 0
#endif

#endif
