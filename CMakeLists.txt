cmake_minimum_required(VERSION 3.29)
project(loop-convert)
set(LLVM_DIR "/usr/lib/llvm-18")

set(CLANG_SEARCH_PATHS
  ${LLVM_DIR}
  ${LLVM_DIR}/lib/cmake
  ${LLVM_DIR}/lib/cmake/llvm
  ${LLVM_DIR}/lib/cmake/clang
  ${LLVM_DIR}/share/clang/cmake
  ${LLVM_DIR}/share/llvm/cmake
)
find_package(Clang
  REQUIRED
  CONFIG
  PATHS ${CLANG_SEARCH_PATHS}
  NO_DEFAULT_PATH
)

include_directories(${CLANG_INCLUDE_DIRS})

set(LLVM_LINK_COMPONENTS support)
add_executable(loop-convert
  main.cpp
  )
target_link_libraries(loop-convert
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )