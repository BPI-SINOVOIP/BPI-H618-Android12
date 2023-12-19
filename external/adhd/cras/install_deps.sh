#!/bin/sh
apt-get install -y \
  automake \
  build-essential \
  cmake \
  g++ \
  gdb \
  git \
  ladspa-sdk \
  libasound-dev \
  libdbus-1-dev \
  libncurses5-dev \
  libsbc-dev \
  libsndfile-dev \
  libspeexdsp-dev \
  libtool \
  libudev-dev \
  wget \
  zip
cd /tmp
git clone https://github.com/ndevilla/iniparser.git
cd iniparser
make
cp libiniparser.* /usr/local/lib
cp src/dictionary.h src/iniparser.h /usr/local/include
chmod 644 /usr/local/include/dictionary.h /usr/local/include/iniparser.h
chmod 644 /usr/local/lib/libiniparser.a
chmod 755 /usr/local/lib/libiniparser.so.*

cd /tmp
git clone https://github.com/google/googletest.git -b v1.8.x
cd googletest
mkdir build
cd build
cmake .. -DBUILD_SHARED_LIBS=ON \
  -DINSTALL_GTEST=ON \
  -DCMAKE_INSTALL_PREFIX:PATH=/usr
make
make install

# Need to build and install alsa so there is a static lib.
mkdir -p /tmp/alsa-build &&
  cd /tmp/alsa-build && \
  wget ftp://ftp.alsa-project.org/pub/lib/alsa-lib-1.1.4.1.tar.bz2 && \
  bzip2 -f -d alsa-lib-* && \
  tar xf alsa-lib-* && \
  cd alsa-lib-* && \
  ./configure --enable-static --disable-shared && \
  make clean && \
  make -j$(nproc) all && \
  make install
