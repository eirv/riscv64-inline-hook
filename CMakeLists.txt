cmake_minimum_required(VERSION 3.22.1)

project("rv64hook")

set(CMAKE_CXX_STANDARD 20)

set(RV64HOOK_SOURCES
        src/rv64hook.cc
        src/hook_handle.cc
        src/hook_locker.cc
        src/instruction_relocator.cc
        src/logger.cc
        src/memory_allocator.cc
        src/scoped_rwx_memory.cc
        src/trampoline.cc)

add_subdirectory(external/berberis)

add_library(${PROJECT_NAME} SHARED ${RV64HOOK_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE berberis-static)

add_library(${PROJECT_NAME}-static STATIC ${RV64HOOK_SOURCES})
target_include_directories(${PROJECT_NAME}-static PUBLIC include)
target_include_directories(${PROJECT_NAME}-static PRIVATE src)
target_link_libraries(${PROJECT_NAME}-static PRIVATE berberis-static)
