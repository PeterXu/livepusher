#!/bin/bash

ROOT=$(pwd)

if [ "$1" = "clean" ]; then
    make distclean
    exit 0
elif [ "$1" = "distclean" ]; then
    make distclean
    rm -rf out
    exit 0
fi

if [ "$1" = "arm64" ]; then
    ARCH=aarch64
    HOST=${ARCH}-linux-android
    CROSS_PREFIX=${ARCH}-linux-android-
    TOOLCHANIN=${ARCH}-linux-android-4.9
    ARCH=$1
    ABI="arm64-v8a"
    HOST=arm-linux
else
    ARCH=arm
    HOST=${ARCH}-linux-androideabi
    CROSS_PREFIX=${ARCH}-linux-androideabi-
    TOOLCHANIN=${ARCH}-linux-androideabi-4.9
    ABI="armeabi-v7a"
    HOST=$ARCH-linux
fi

SYSROOT=$ANDROID_NDK/platforms/android-21/arch-${ARCH}

PATH=$PATH:$ANDROID_NDK/toolchains/$TOOLCHANIN/prebuilt/linux-x86_64/bin

export CC="${CROSS_PREFIX}gcc --sysroot=$SYSROOT"
export CXX="${CROSS_PREFIX}g++ --sysroot=$SYSROOT"
export LDFLAGS="-Wl,-rpath-link=$SYSROOT/usr/lib -L$SYSROOT/usr/lib -nostdlib -lc -lm -ldl -llog -lgcc"

./configure \
    --prefix=$ROOT/out/$ABI \
    --enable-shared \
    --enable-static \
    --with-pic \
    --without-mp4v2 \
    --host=$HOST

make install


exit 0
