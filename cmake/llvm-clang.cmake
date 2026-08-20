# LLVM + Clang toolchain for dcmmlib.
#   cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-clang.cmake ..

set(CMAKE_C_COMPILER clang CACHE STRING "C compiler")
set(CMAKE_CXX_COMPILER clang++ CACHE STRING "C++ compiler")
set(CMAKE_ASM_COMPILER clang CACHE STRING "ASM compiler")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
