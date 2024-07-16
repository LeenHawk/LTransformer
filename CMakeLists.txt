cmake_minimum_required(VERSION 3.30)
project(LTransformer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

add_subdirectory(LoopExtract)
add_subdirectory(DeRecursion)