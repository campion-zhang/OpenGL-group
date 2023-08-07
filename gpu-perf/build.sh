#!/bin/bash

if [[ -a build ]]; then
    cd ./build
    make clean
    cd ../
    rm -rf ./build
fi

mkdir build && cd build

cmake ../ -DUSE_XEGL=1 -DCMAKE_BUILD_TYPE=Debug  -DCUSTOM_EGL_PATH=/home/sietium/XYGPU/xygpu_driver/build/gcc-arm-linux-internal-indirect-debug-phytium-xygpu400-r1p0-gles20-gles11-linux/lib/  -DCUSTOM_INCLUDE_PATH=/home/sietium/XYGPU/xygpu_driver/3rdparty/include/khronos/

make -j8
