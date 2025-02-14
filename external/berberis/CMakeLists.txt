cmake_minimum_required(VERSION 3.22.1)

project("berberis")

add_library(${PROJECT_NAME}-static STATIC
        src/base/exec_region.cc
        src/base/mmap_posix.cc
        src/assembler/machine_code.cc)
target_include_directories(${PROJECT_NAME}-static PUBLIC include)
