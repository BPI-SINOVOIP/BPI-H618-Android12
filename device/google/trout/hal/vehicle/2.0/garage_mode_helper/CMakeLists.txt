cmake_minimum_required(VERSION 3.13)

project(garage_mode_tool CXX)

set(_trout_GMHELPER_CXX_FLAGS_ -Wall -Werror -std=c++17)

add_executable(garage_mode_helper helper.cpp)
target_compile_options(garage_mode_helper PRIVATE ${_trout_GMHELPER_CXX_FLAGS_})
