#!/bin/bash

CC=`ndk-which gcc`
ROOT=`dirname $CC`

ARCH=arm            # aarch64,arm
PREFIX=armeabi-v7a  # arm64-v8a,armeabi,armeabi-v7a,armeabi-v7a-neon
HOST=$ARCH-linux

export SYSROOT=$ANDROID_NDK/platforms/android-15/arch-arm
export PATH=$PATH:$ROOT
export CC="$CC --sysroot=$SYSROOT"
export CXX="$(ndk-which g++) --sysroot=$SYSROOT"

./configure \
    --prefix=$(pwd)/../libs/faac/$PREFIX \
    --includedir=$(pwd)/../libs/faac/include \
    --enable-shared \
    --enable-static \
    --with-pic \
    --without-mp4v2 \
    --host=$HOST

make
