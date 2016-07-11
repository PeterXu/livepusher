#!/bin/bash

ROOT=$(pwd)
CC=`ndk-which gcc`
CCPATH=`dirname $CC`

ARCH=${ARCH:-arm}               # aarch64,arm
EABI=${EABI:-armeabi-v7a}       # arm64-v8a,armeabi,armeabi-v7a,armeabi-v7a-neon
HOST=$ARCH-linux
SYSROOT=$ANDROID_NDK/platforms/android-9/arch-$ARCH

export PATH=$PATH:$CCPATH
export CC="$CC --sysroot=$SYSROOT"
export CXX="$(ndk-which g++) --sysroot=$SYSROOT"
export LDFLAGS="-Wl,-rpath-link=$SYSROOT/usr/lib -L$SYSROOT/usr/lib -nostdlib -lc -lm -ldl -llog -lgcc"


PREFIX="$ROOT/../libs/faac/$EABI"
INCDIR="$ROOT/../libs/faac/include"
rm -rf $PREFIX $INCDIR


./configure \
    --prefix=$PREFIX \
    --includedir=$INCDIR \
    --enable-shared \
    --enable-static \
    --with-pic \
    --without-mp4v2 \
    --host=$HOST

make clean || true
make install


exit 0
