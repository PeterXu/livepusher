#!/bin/bash

#TOOLCHAIN_VER=4.9
if [ -n $(which ndk-build) ]; then
    NDK_PATH=$(dirname $(which ndk-build))
else
    echo "  ndk-build not found"
    exit 1
fi

while read line; do
    if [[ $line =~ ^APP_ABI\ := ]]; then
        archs=(${line#*=})
        if [[ " ${archs[*]} " == *" all "* ]]; then
            build_all=true
        fi
        break
    fi
done <"Application.mk"

if [ -z "$archs" ]; then
    echo "Application.mk no archs, please add 'APP_ABI:=<ARCH>'"
    exit 1
else
    echo "cpu arch: ${archs[@]}"
fi

PLATFORM_VERSION=9
while read line; do
    if [[ $line =~ ^APP_PLATFORM\ *?:= ]]; then
        PLATFORM_VERSION=${line#*-}
        break
    fi
done <"Application.mk"

if [ ! -d "$NDK_PATH/platforms/android-$PLATFORM_VERSION" ]; then
    echo "${NDK_PATH/platforms/android-$PLATFORM_VERSION}  not found"
    exit 1
fi
PLATFORM_VERSION=android-$PLATFORM_VERSION


OS=`uname -s | tr '[A-Z]' '[a-z]'`
if [[ `uname -a` =~ "x86_64" ]]; then
    BIT=x86_64
else
    BIT=x86
fi
echo "current system:" $OS-$BIT


function build_toolchain_version
{
	echo "**********************************************************"
    echo "*********************Config compiler**********************"
    echo "**********************************************************"

	folders=$NDK_PATH/toolchains/$TOOLCHAIN_NAME-*
    for i in $folders; do
        n=${i#*$NDK_PATH/toolchains/$TOOLCHAIN_NAME-}
        reg='.*?[a-zA-Z].*?'
        if ! [[ $n =~ $reg ]] ; then
            if [  -d $NDK_PATH/toolchains/$TOOLCHAIN_NAME-$n/prebuilt ]; then
                TOOLCHAIN_VER=$n
            fi
        fi
    done
    if [ ! -d $NDK_PATH/toolchains/$TOOLCHAIN_NAME-$TOOLCHAIN_VER ]; then
        echo "$NDK_PATH/toolchains/$TOOLCHAIN_NAME-$TOOLCHAIN_VER not found"
        exit 1
    fi

	echo "using $NDK_PATH/toolchains/$ARCH-$TOOLCHAIN_VER"
}

function build_init
{
    echo "**********************************************************"
    echo "************************Config Env************************"
    echo "**********************************************************"

    TOOLCHAINS_DIR=`pwd`/include/android-toolchain/$EABIARCH
    echo $TOOLCHAINS_DIR
    if [ ! -d $TOOLCHAINS_DIR ]; then
        $NDK_PATH/build/tools/make-standalone-toolchain.sh platform=$PLATFORM_VERSION --install-dir=`pwd`/include/android-toolchain/$EABIARCH --toolchain=$PREBUILT
    fi
}

function build_x264
{
    echo "*****************************************************"
    echo "*********************Compile x264********************"
    echo "*****************************************************"

    find include/x264/ -name "*.o" -type f -delete

    echo toolchains: $TOOLCHAINS_DIR
    echo prefix: $PREFIX
    export PATH=${PATH}:$TOOLCHAINS_DIR/bin/
    CROSS_COMPILE=$TOOLCHAINS_DIR/bin/$EABIARCH-
    echo compiler: $CROSS_COMPILE
    CFLAGS=$OPTIMIZE_CFLAGS
    ADDITIONAL_CONFIGURE_FLAG="$ADDITIONAL_CONFIGURE_FLAG"
    export CPPFLAGS="$CFLAGS"
    export CFLAGS="$CFLAGS"
    export CXXFLAGS="$CFLAGS"
    export CXX="${CROSS_COMPILE}g++ --sysroot=$TOOLCHAINS_DIR/sysroot"
    export AS="${CROSS_COMPILE}gcc --sysroot=$TOOLCHAINS_DIR/sysroot"
    export CC="${CROSS_COMPILE}gcc --sysroot=$TOOLCHAINS_DIR/sysroot"
    export NM="${CROSS_COMPILE}nm"
    export STRIP="${CROSS_COMPILE}strip"
    export RANLIB="${CROSS_COMPILE}ranlib"
    export AR="${CROSS_COMPILE}ar"
    export LDFLAGS="-Wl,-rpath-link=$TOOLCHAINS_DIR/sysroot/usr/lib -L$TOOLCHAINS_DIR/sysroot/usr/lib -nostdlib -lc -lm -ldl -llog -lgcc"

    cd include/x264
    ./configure --prefix=$(pwd)/../libs/x264/$PREFIX --includedir=$(pwd)/../libs/x264/include --disable-gpac  --host=$ARCH-linux --enable-pic --enable-static $ADDITIONAL_CONFIGURE_FLAG || exit 1
    make clean || exit 1
    make STRIP= -j4 install || exit 1
    cd ../../
}

function build_faac
{
    echo "*****************************************************"
    echo "********************Compile faac*********************"
    echo "*****************************************************"

    find include/faac/ -name "*.o" -type f -delete

    TOOLCHAINS_DIR=`pwd`/include/android-toolchain/$EABIARCH
    echo toolchains: $TOOLCHAINS_DIR
    echo prefix: $PREFIX
    export PATH=${PATH}:$TOOLCHAINS_DIR/bin/
    CROSS_COMPILE=$TOOLCHAINS_DIR/bin/$EABIARCH-
    echo compiler: $CROSS_COMPILE
    CFLAGS=$OPTIMIZE_CFLAGS
    ADDITIONAL_CONFIGURE_FLAG="$ADDITIONAL_CONFIGURE_FLAG --enable-gpl"
    export CPPFLAGS="$CFLAGS"
    export CFLAGS="$CFLAGS"
    export CXXFLAGS="$CFLAGS"
    export CXX="${CROSS_COMPILE}g++ --sysroot=$TOOLCHAINS_DIR/sysroot"
    export AS="${CROSS_COMPILE}gcc --sysroot=$TOOLCHAINS_DIR/sysroot"
    export CC="${CROSS_COMPILE}gcc --sysroot=$TOOLCHAINS_DIR/sysroot"
    export NM="${CROSS_COMPILE}nm"
    export STRIP="${CROSS_COMPILE}strip"
    export RANLIB="${CROSS_COMPILE}ranlib"
    export AR="${CROSS_COMPILE}ar"
    export LDFLAGS="-Wl,-rpath-link=$TOOLCHAINS_DIR/sysroot/usr/lib -L$TOOLCHAINS_DIR/sysroot/usr/lib -nostdlib -lc -lm -ldl -llog -lgcc"

    cd include/faac
    ./configure --prefix=$(pwd)/../libs/faac/$PREFIX --includedir=$(pwd)/../libs/faac/include --disable-shared --enable-static --with-pic --host=$ARCH-linux --without-mp4v2  $ADDITIONAL_CONFIGURE_FLAG || exit 1
    make clean || exit 1
    make install
    cp -v libfaac/.libs/libfaac.a $(pwd)/../libs/faac/$PREFIX/lib
    cp ./include/*.h $(pwd)/../libs/faac/$PREFIX/include
    cd ../../
}

function build_rtmp
{
    echo "*****************************************************"
    echo "********************Compile rtmp*********************"
    echo "*****************************************************"

    find include/rtmpdump/ -name "*.o" -type f -delete

    TOOLCHAINS_DIR=`pwd`/include/android-toolchain/$EABIARCH
    echo toolchains: $TOOLCHAINS_DIR
    echo prefix: $PREFIX
    export PATH=${PATH}:$TOOLCHAINS_DIR/bin/
    CROSS_COMPILE=$TOOLCHAINS_DIR/bin/$EABIARCH-
    echo compiler: $CROSS_COMPILE
    CFLAGS=$OPTIMIZE_CFLAGS
    ADDITIONAL_CONFIGURE_FLAG="$ADDITIONAL_CONFIGURE_FLAG"
    #CFLAGS=" -I$ARM_INC -fpic -DANDROID -fpic -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__  -Wno-psabi -march=armv5te -mtune=xscale -msoft-float -mthumb -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64 -DANDROID  -Wa,--noexecstack -MMD -MP "
    cd include/rtmpdump
    make install SYS=android prefix=`pwd`/../libs/rtmpdump/$PREFIX incdir=$(pwd)/../libs/rtmpdump/include CRYPTO= SHARED=  XDEF=-DNO_SSL CROSS_COMPILE=$EABIARCH-  INC="$CFLAGS -I$TOOLCHAINS_DIR/sysroot/usr/include" XCFLAGS="--sysroot=$TOOLCHAINS_DIR/sysroot -fpic" XLDFLAGS="-L$TOOLCHAINS_DIR/sysroot/usr/lib"
    cd ../../
}


#=======================================================================

#arm v5
if [[ " ${archs[*]} " == *" armeabi "* ]] || [ "$build_all" = true ]; then
    EABIARCH=arm-linux-androideabi
    ARCH=armv5
    CPU=armv5
    OPTIMIZE_CFLAGS="-marm -fPIC"
    PREFIX=armeabi
    TOOLCHAIN_NAME=arm-linux-androideabi
    build_toolchain_version
    PREBUILT=arm-linux-androideabi-$TOOLCHAIN_VER
    #if [ ! -d "$PREBUILT" ]; then PREBUILT="$PREBUILT"_64; fi
    # If you want x264, compile armv6
    build_init
    build_x264
    build_faac
    build_rtmp
fi

#x86
if [[ " ${archs[*]} " == *" x86 "* ]] || [ "$build_all" = true ]; then
    EABIARCH=i686-linux-android
    ARCH=x86
    OPTIMIZE_CFLAGS="-m32 -fPIC"
    PREFIX=x86
    ADDITIONAL_CONFIGURE_FLAG=--disable-asm
    TOOLCHAIN_NAME=x86
    build_toolchain_version
    PREBUILT=x86-$TOOLCHAIN_VER
    #if [ ! -d "$PREBUILT" ]; then PREBUILT="$PREBUILT"_64; fi
    build_init    
    build_x264
    build_faac
    build_rtmp
fi

#mips
if [[ " ${archs[*]} " == *" mips "* ]] || [ "$build_all" = true ]; then
    EABIARCH=mipsel-linux-android
    ARCH=mips
    OPTIMIZE_CFLAGS="-EL -march=mips32 -mips32 -mhard-float -fPIC"
    PREFIX=mips
    ADDITIONAL_CONFIGURE_FLAG="--disable-mipsdspr1 --disable-mipsdspr2 --disable-asm"
    TOOLCHAIN_NAME=mipsel-linux-android
    build_toolchain_version
    PREBUILT=mipsel-linux-android-$TOOLCHAIN_VER
    #if [ ! -d "$PREBUILT" ]; then PREBUILT="$PREBUILT"_64; fi
    build_init    
    build_x264
    build_faac
    build_rtmp
fi

if [[ " ${archs[*]} " == *" armeabi-v7a "* ]] || [ "$build_all" = true ]; then
    #arm v7vfpv3
    EABIARCH=arm-linux-androideabi
    ARCH=arm
    CPU=armv7-a
    OPTIMIZE_CFLAGS="-mfloat-abi=softfp -mfpu=vfpv3-d16 -marm -march=$CPU -fPIC"
    PREFIX=armeabi-v7a
    TOOLCHAIN_NAME=arm-linux-androideabi
    build_toolchain_version
    PREBUILT=arm-linux-androideabi-$TOOLCHAIN_VER
    #if [ ! -d "$PREBUILT" ]; then PREBUILT="$PREBUILT"_64; fi
    build_init    
    build_x264
    build_faac
    build_rtmp

    #arm v7 + neon (neon also include vfpv3-32)
    EABIARCH=arm-linux-androideabi
    ARCH=arm
    CPU=armv7-a
    OPTIMIZE_CFLAGS="-mfloat-abi=softfp -mfpu=neon -marm -march=$CPU -mtune=cortex-a8 -mthumb -D__thumb__ -fPIC"
    PREFIX=armeabi-v7a-neon
    ADDITIONAL_CONFIGURE_FLAG=--enable-neon
    PREBUILT=arm-linux-androideabi-$TOOLCHAIN_VER
    #if [ ! -d "$PREBUILT" ]; then PREBUILT="$PREBUILT"_64; fi 
    #build_init 
    build_x264
    build_faac
    build_rtmp
fi

if [[ " ${archs[*]} " == *" arm64-v8a "* ]] || [ "$build_all" = true ]; then
    EABIARCH=aarch64-linux-android
    ARCH=aarch64
    #CPU=arm64
    OPTIMIZE_CFLAGS="-fPIC"
    PREFIX=arm64-v8a
    TOOLCHAIN_NAME=aarch64-linux-android
    build_toolchain_version
    PREBUILT=aarch64-linux-android-$TOOLCHAIN_VER
    build_init    
    build_x264
    ARCH=arm
    build_faac
    build_rtmp
fi

if [[ " ${archs[*]} " == *" x86_64 "* ]] || [ "$build_all" = true ]; then
    EABIARCH=x86_64-linux-android
    ARCH=x86_64
    OPTIMIZE_CFLAGS="-fPIC"
    PREFIX=x86_64
    ADDITIONAL_CONFIGURE_FLAG=--disable-asm
    TOOLCHAIN_NAME=x86_64
    build_toolchain_version
    PREBUILT=x86_64-$TOOLCHAIN_VER
    build_init    
    build_x264
    build_faac
    build_rtmp
fi

exit 0
