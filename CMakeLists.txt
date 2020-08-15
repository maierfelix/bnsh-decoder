cmake_minimum_required(VERSION 3.10)

project(
  BNSHDecompiler
    DESCRIPTION "BNSH shader decompiler standalone"
    LANGUAGES C CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE not set, forcing it to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

set(BNSH_DECOMPILER_SRC_DIR "${BNSHDecompiler_SOURCE_DIR}/src")
set(BNSH_DECOMPILER_INCLUDE_DIR "${DECOMPILER_SRC_DIR}/include")

set(DUMMY_FILE "${BNSH_DECOMPILER_SRC_DIR}/Dummy.cpp")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

set(EMSCRIPTEN_MODULE_FLAGS "")

if (${CMAKE_SYSTEM_NAME} MATCHES Emscripten)
    message(STATUS "Building using Emscripten")
    #set(CMAKE_C_COMPILER "emcc")
    #set(CMAKE_CXX_COMPILER "em++")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s WASM=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -s WASM=1")
endif ()

# allow modules to include each other
include_directories(src)

# TODO: bad idea
include_directories(third_party/fmt/include)

# add third party libraries
add_subdirectory(third_party)

add_subdirectory(src/common)
add_subdirectory(src/video_core)
add_subdirectory(src/bnsh_cli)
