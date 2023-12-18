#!/bin/bash

LICHEE_DIR=${LICHEE_TOP_DIR}
LICHEE_BUILD_CONFIG=${LICHEE_DIR}/.buildconfig

if [ -f "${LICHEE_BUILD_CONFIG}" ]; then
    source ${LICHEE_BUILD_CONFIG}
    export KDIR=${LICHEE_TOP_DIR}/out/kernel/build/
    if [ "$LICHEE_ARCH" == "arm64" ]; then
        export CROSS_COMPILE=${LICHEE_TOOLCHAIN_PATH}/bin/aarch64-linux-gnu-
    else
        export CROSS_COMPILE=${LICHEE_TOOLCHAIN_PATH}/bin/arm-linux-gnueabi-
    fi
else
    echo "need config and build kernel first!"
    exit 0
fi

VS_BUILD_OUT=${LICHEE_PLAT_OUT}/vs
SRC_PATH=${VS_BUILD_OUT}/vs-modules

SRC_DIR=Utility
cp -rf ${SRC_PATH}/Util_modules.makefile ${SRC_PATH}/Utility
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/Util_modules.makefile
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/Util_modules.makefile clean


SRC_DIR=HAL_SX6
cp -rf ${SRC_PATH}/kernel_modules.makefile ${SRC_PATH}/HAL_SX6
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/kernel_modules.makefile
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/kernel_modules.makefile clean

cp -rf ${SRC_PATH}/Hal_modules.makefile ${SRC_PATH}/HAL_SX6
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/Hal_modules.makefile
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/Hal_modules.makefile clean

SRC_DIR=Tools
cp -rf ${SRC_PATH}/Tools_modules.makefile ${SRC_PATH}/Tools
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/Tools_modules.makefile
make -C ${SRC_PATH}/${SRC_DIR} -f ${SRC_PATH}/Tools_modules.makefile clean


unset KDIR
unset CROSS_COMPILE
