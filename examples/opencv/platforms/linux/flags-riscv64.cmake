# Compiler flags for RISC-V 64-bit Linux builds
# This file is included early by cmake/platforms/OpenCV-Linux.cmake
# Note: add_extra_compiler_option is not yet available at this point.

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-march=rv64gc)
endif()
