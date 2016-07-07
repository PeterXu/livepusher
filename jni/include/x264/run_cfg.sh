#!/bin/bash

CC=`ndk-which gcc`
ROOT=`dirname $CC`

ARCH=arm            # aarch64,arm
PREFIX=armeabi-v7a  # arm64-v8a,armeabi,armeabi-v7a,armeabi-v7a-neon
HOST=$ARCH-linux-androideabi

export SYSROOT=$ANDROID_NDK/platforms/android-15/arch-arm
export PATH=$PATH:$ROOT
export CC="$CC --sysroot=$SYSROOT"

./configure \
    --prefix=$(pwd)/../libs/x264/$PREFIX \
    --includedir=$(pwd)/../libs/x264/include \
    --disable-cli \
    --enable-shared \
    --enable-static \
    --bit-depth=8 \
    --chroma-format=420 \
    --enable-strip \
    --enable-pic \
    --disable-avs \
    --disable-ffms \
    --disable-gpac \
    --host=$HOST \
    --sysroot=$SYSROOT \
    --cross-prefix=$HOST-

make
