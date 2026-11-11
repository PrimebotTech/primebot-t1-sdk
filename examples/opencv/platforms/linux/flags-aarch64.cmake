# Compiler flags for AArch64 (ARM64) Linux builds
# This file is included early by cmake/platforms/OpenCV-Linux.cmake
# Note: add_extra_compiler_option is not yet available at this point,
# so we use standard CMake commands instead.

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Use generic ARMv8-A flags for broad compatibility
  add_compile_options(-march=armv8-a)
endif()
