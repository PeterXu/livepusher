#!/bin/bash

CC=`ndk-which gcc`
ROOT=`dirname $CC`

ARCH=arm            # aarch64,arm
PREFIX=armeabi-v7a  # arm64-v8a,armeabi,armeabi-v7a,armeabi-v7a-neon
HOST=$ARCH-linux-androideabi

export SYSROOT=$ANDROID_NDK/platforms/android-15/arch-arm
export PATH=$PATH:$ROOT
export CC="$CC --sysroot=$SYSROOT"
export CXX="$(ndk-which g++) --sysroot=$SYSROOT"

make install SYS=android prefix=`pwd`/../libs/rtmpdump/$PREFIX incdir=$(pwd)/../libs/rtmpdump/include CRYPTO= SHARED=  XDEF=-DNO_SSL CROSS_COMPILE=$HOST-  INC="$CFLAGS -I$SYSROOT/usr/include" XCFLAGS="--sysroot=$SYSROOT -fpic" XLDFLAGS="-L$SYSROOT/usr/lib --sysroot=$SYSROOT"

