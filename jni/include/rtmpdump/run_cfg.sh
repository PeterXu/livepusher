#!/bin/bash

ROOT=$(pwd)
CC=`ndk-which gcc`
CCPATH=`dirname $CC`

ARCH=${ARCH:-arm}               # aarch64,arm
EABI=${EABI:-armeabi-v7a}       # arm64-v8a,armeabi,armeabi-v7a,armeabi-v7a-neon
HOST=$ARCH-linux-androideabi
SYSROOT=$ANDROID_NDK/platforms/android-9/arch-$ARCH

export PATH=$PATH:$CCPATH
export CC="$CC --sysroot=$SYSROOT"
export CXX="$(ndk-which g++) --sysroot=$SYSROOT"



PREFIX="$ROOT/../libs/rtmpdump/$EABI"
INCDIR="$ROOT/../libs/rtmpdump/include"
rm -rf $PREFIX $INCDIR

make clean
make install SYS=android prefix=$PREFIX incdir=$INCDIR CRYPTO= SHARED=  XDEF=-DNO_SSL CROSS_COMPILE=$HOST-  INC="$CFLAGS -I$SYSROOT/usr/include" XCFLAGS="--sysroot=$SYSROOT -fpic" XLDFLAGS="--sysroot=$SYSROOT"

exit 0
