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
export LDFLAGS="-Wl,-rpath-link=$SYSROOT/usr/lib -L$SYSROOT/usr/lib -nostdlib -lc -lm -ldl -llog -lgcc"


PREFIX="$ROOT/../libs/x264/$EABI"
INCDIR="$ROOT/../libs/x264/include"
rm -rf $PREFIX $INCDIR

./configure \
    --prefix=$PREFIX \
    --includedir=$INCDIR \
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

make clean
make install

exit 0
