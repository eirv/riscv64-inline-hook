cmake_minimum_required(VERSION 3.22.1)

project("rv64hook")

enable_language(ASM)

set(CMAKE_CXX_STANDARD 20)

option(RV64HOOK_BUILD_SHARED "Build shared library" OFF)
option(RV64HOOK_BUILD_STATIC "Build static library" ON)
option(RV64HOOK_BUILD_TRAMPOLINE "Automatically build trampoline" ON)

if (DEFINED ANDROID_ABI)
    set(RV64HOOK_ABI ${ANDROID_ABI})
else ()
    set(RV64HOOK_ABI ${CMAKE_SYSTEM_PROCESSOR})
endif ()

set(RV64HOOK_BERBERIS_DIR external/berberis)

set(RV64HOOK_SOURCES
        src/libc/memcpy_generic.cc
        src/libc/syscalls.cc
        src/core/rv64hook.cc
        src/core/function_record.cc
        src/core/hook_handle.cc
        src/core/hook_locker.cc
        src/core/logger.cc
        src/core/memory.cc
        src/core/scoped_rwx_memory.cc)
set(RV64HOOK_INCLUDES include compat)
set(RV64HOOK_PRIVATE_INCLUDES src)
set(RV64HOOK_DEFINITIONS)

if (RV64HOOK_ABI STREQUAL "riscv64")
    set(RV64HOOK_SUPPORTED ON)
    set(RV64HOOK_SOURCES
            ${RV64HOOK_SOURCES}
            src/arch/riscv64/riscv64_support.cc
            src/libc/memcpy_vext.S
            ${RV64HOOK_BERBERIS_DIR}/src/base/exec_region.cc
            ${RV64HOOK_BERBERIS_DIR}/src/base/mmap_posix.cc
            ${RV64HOOK_BERBERIS_DIR}/src/assembler/machine_code.cc)
    set(RV64HOOK_PRIVATE_INCLUDES
            ${RV64HOOK_PRIVATE_INCLUDES}
            ${RV64HOOK_BERBERIS_DIR}/include)
    if (RV64HOOK_BUILD_TRAMPOLINE)
        set(RV64HOOK_SOURCES ${RV64HOOK_SOURCES} src/arch/riscv64/trampoline_riscv64.S)
    endif ()
else ()
    add_library(${PROJECT_NAME} INTERFACE)
    target_include_directories(${PROJECT_NAME} INTERFACE compat)
    add_library(${PROJECT_NAME}-static INTERFACE)
    target_include_directories(${PROJECT_NAME}-static INTERFACE compat)
endif ()

if (DEFINED RV64HOOK_SUPPORTED)
    if (RV64HOOK_BUILD_TRAMPOLINE)
        set(RV64HOOK_DEFINITIONS ${RV64HOOK_DEFINITIONS} RV64HOOK_BUILD_TRAMPOLINE)
    endif ()
    if (RV64HOOK_BUILD_SHARED)
        add_library(${PROJECT_NAME} SHARED ${RV64HOOK_SOURCES})
        target_include_directories(${PROJECT_NAME} PUBLIC ${RV64HOOK_INCLUDES})
        target_include_directories(${PROJECT_NAME} PRIVATE ${RV64HOOK_PRIVATE_INCLUDES})
        target_compile_definitions(${PROJECT_NAME} PRIVATE ${RV64HOOK_DEFINITIONS})
    endif ()
    if (RV64HOOK_BUILD_STATIC)
        add_library(${PROJECT_NAME}-static STATIC ${RV64HOOK_SOURCES})
        target_include_directories(${PROJECT_NAME}-static PUBLIC ${RV64HOOK_INCLUDES})
        target_include_directories(${PROJECT_NAME}-static PRIVATE ${RV64HOOK_PRIVATE_INCLUDES})
        target_compile_definitions(${PROJECT_NAME}-static PRIVATE ${RV64HOOK_DEFINITIONS})
    endif ()
endif ()
