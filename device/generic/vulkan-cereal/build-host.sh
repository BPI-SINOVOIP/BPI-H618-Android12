mkdir build
cd build
cmake . ../ -DCMAKE_TOOLCHAIN_FILE=../toolchain/toolchain-linux-x86_64.cmake
make -j24
