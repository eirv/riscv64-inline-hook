cmake_minimum_required(VERSION 3.22.1)

project("berberis-static")

add_library(${PROJECT_NAME} STATIC
        src/base/exec_region.cc
        src/base/mmap_posix.cc
        src/assembler/machine_code.cc
)
target_include_directories(${PROJECT_NAME} PUBLIC include)
