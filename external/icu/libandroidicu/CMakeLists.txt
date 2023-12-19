# Minimum CMake required
cmake_minimum_required(VERSION 3.1.3)

# Project
project(libandroidicu C CXX)

set(ICU_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(ICU_SHIM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/static_shim")

# Glob icu source files
file(
   GLOB_RECURSE
   ICU_CC_SOURCES
   "${ICU_SOURCE_DIR}/*.cpp"
   "${ICU_SOURCE_DIR}/*.h"
)
message(STATUS "ICU_CC_SOURCES=${ICU_CC_SOURCES}")

file(
   GLOB_RECURSE
   ICU_CC_SHIM_SOURCES
   "${ICU_SHIM_SOURCE_DIR}/*.cpp"
   "${ICU_SHIM_SOURCE_DIR}/*.h"
)
message(STATUS "ICU_CC_SHIM_SOURCES=${ICU_CC_SHIM_SOURCES}")

add_library(
   # name
   libandroidicu

   # SHARED or STATIC
   STATIC

   # Provides a relative path to your source file(s).
   ${ICU_CC_SOURCES}
   ${ICU_CC_SHIM_SOURCES}
)
target_include_directories(libandroidicu PRIVATE ${ICU_SOURCE_DIR})
target_include_directories(libandroidicu PRIVATE ${ICU_SHIM_SOURCE_DIR})

set_target_properties(libandroidicu PROPERTIES LINKER_LANGUAGE CXX)
