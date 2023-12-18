#!/bin/sh
rm -rf CMakeCache.txt  CMakeFiles  cmake_install.cmake Makefile
cmake ../newhwc -DCMAKE_TOOLCHAIN_FILE=./cmake.tools

