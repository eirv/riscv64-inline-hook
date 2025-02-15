cmake_minimum_required(VERSION 3.22.1)

project("rv64hook")

enable_language(ASM)

set(CMAKE_CXX_STANDARD 20)

if (DEFINED ANDROID_ABI)
    set(RV64HOOK_ABI ${ANDROID_ABI})
else ()
    set(RV64HOOK_ABI ${CMAKE_SYSTEM_PROCESSOR})
endif ()

set(RV64HOOK_SOURCES
        src/libc/memcpy_generic.cc
        src/rv64hook.cc
        src/function_record.cc
        src/hook_handle.cc
        src/hook_locker.cc
        src/logger.cc
        src/memory.cc
        src/scoped_rwx_memory.cc)
set(RV64HOOK_LIBS)

if (RV64HOOK_ABI STREQUAL "riscv64")
    set(RV64HOOK_SUPPORTED ON)
    set(RV64HOOK_SOURCES
            ${RV64HOOK_SOURCES}
            src/arch/riscv64/riscv64_support.cc
            src/libc/memcpy_vext.S)
    set(RV64HOOK_LIBS ${RV64HOOK_LIBS} berberis-static)
    add_subdirectory(external/berberis)
else ()
    message(WARNING "[riscv64-inline-hook] Unsupported ABI: ${RV64HOOK_ABI}")
endif ()

if (DEFINED RV64HOOK_SUPPORTED)
    option(RV64HOOK_BUILD_SHARED "Build shared library" OFF)
    option(RV64HOOK_BUILD_STATIC "Build static library" ON)

    if (RV64HOOK_BUILD_SHARED)
        add_library(${PROJECT_NAME} SHARED ${RV64HOOK_SOURCES})
        target_include_directories(${PROJECT_NAME} PUBLIC include)
        target_include_directories(${PROJECT_NAME} PRIVATE src)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${RV64HOOK_LIBS})
    endif ()

    if (RV64HOOK_BUILD_STATIC)
        add_library(${PROJECT_NAME}-static STATIC ${RV64HOOK_SOURCES})
        target_include_directories(${PROJECT_NAME}-static PUBLIC include)
        target_include_directories(${PROJECT_NAME}-static PRIVATE src)
        target_link_libraries(${PROJECT_NAME}-static PRIVATE ${RV64HOOK_LIBS})
    endif ()
endif ()
