#!/bin/bash

ROOT=$(pwd)

if [ "$1" = "clean" ]; then
    make clean
    exit 0
fi

if [ "$1" = "arm64" ]; then
    ARCH=aarch64
    HOST=${ARCH}-linux-android
    CROSS_PREFIX=${ARCH}-linux-android-
    TOOLCHANIN=${ARCH}-linux-android-4.9
    ARCH=$1
    ABI="arm64-v8a"
else
    ARCH=arm
    HOST=${ARCH}-linux-androideabi
    CROSS_PREFIX=${ARCH}-linux-androideabi-
    TOOLCHANIN=${ARCH}-linux-androideabi-4.9
    ABI="armeabi-v7a"
fi

SYSROOT=$ANDROID_NDK/platforms/android-21/arch-${ARCH}

PATH=$PATH:$ANDROID_NDK/toolchains/$TOOLCHANIN/prebuilt/linux-x86_64/bin

PREFIX="$ROOT/out/$ABI"

make install SYS=android prefix=$PREFIX CRYPTO= SHARED=  XDEF=-DNO_SSL CROSS_COMPILE=$HOST-  INC="$CFLAGS -I$SYSROOT/usr/include" XCFLAGS="--sysroot=$SYSROOT -fpic" XLDFLAGS="--sysroot=$SYSROOT"

exit 0
