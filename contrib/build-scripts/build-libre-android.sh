#!/bin/bash

#
# build ZeroMQ for android
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 

if [ ! -f include/re.h ]; then
	echo
	echo "Cannot find include/re.h"
	echo "Run script from within libre directory:"
	echo "re-4.7.0$ ../../../${ME}"
	echo
	exit
fi

if [ -f Makefile ]; then
	make clean
fi

# if [ -f ispatched ]; then
# 	rm ./ispatched
# else
# 	patch -p1 < ../zeromq-3.1.0.android.patch
# fi
# touch ispatched

CC_COMMON_FLAGS="-I. -DANDROID -DTARGET_OS_ANDROID -fno-strict-aliasing -fno-omit-frame-pointer -fno-exceptions -fdata-sections -ffunction-sections"
CPP_COMMON_FLAGS="${CC_COMMON_FLAGS} -fno-rtti"


# build for x86

# CC_X86_FLAGS="${CC_COMMON_FLAGS} \
# -funwind-tables \
# -finline-limit=300"
# CPP_X86_FLAGS="${CPP_COMMON_FLAGS} \
# -funwind-tables \
# -finline-limit=300"
# 
# X86_SYSROOT="${ANDROID_NDK}/platforms/android-9/arch-x86"
# X86_TOOLCHAIN_ROOT="${ANDROID_NDK}/toolchains/x86-4.7/prebuilt/darwin-x86_64"
# X86_TOOL_PREFIX="i686-linux-android"
# X86_COMMON_FLAGS="\
# -isystem ${ANDROID_NDK}/platforms/android-9/arch-x86/usr/include \
# -isystem ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/include \
# -isystem ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/libs/x86/include \
# "
# X86_CPP_STDLIB="${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/libs/x86"
# 
# export ARCH="x86"
# export CPP="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-cpp"
# export CXXCPP="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-cpp"
# export CPPFLAGS="--sysroot=${X86_SYSROOT}"
# export CXXCPPFLAGS="--sysroot=${X86_SYSROOT}"
# export CC="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-gcc"
# export CXX="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-g++"
# export LD="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-ld"
# export CXXFLAGS="-Os -s ${CPP_X86_FLAGS} --sysroot=${X86_SYSROOT} ${X86_COMMON_FLAGS}"
# export CFLAGS="-Os -s ${CC_X86_FLAGS} --sysroot=${X86_SYSROOT} ${X86_COMMON_FLAGS}"
# export LDFLAGS="--sysroot=${X86_SYSROOT} -L${X86_CPP_STDLIB} -lgnustl_static"
# export AR="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-ar"
# export AS="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-as"
# export LIBTOOL="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-libtool"
# export STRIP="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-strip"
# export RANLIB="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-ranlib"
# 
# set +e
# make -j2



# build for arm

CC_ARMEABI_FLAGS="${CC_ARMEABI_FLAGS} \
-D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5__ \
-fsigned-char \
-march=armv5te \
-mtune=xscale \
-msoft-float \
-mfpu=vfp \
-mfloat-abi=softfp \
-fPIC \
-finline-limit=64"


ARM_SYSROOT="${ANDROID_NDK}/platforms/android-8/arch-arm"
ARMEABI_TOOLCHAIN_ROOT="${ANDROID_NDK}/toolchains/arm-linux-androideabi-4.7/prebuilt/darwin-x86_64"
ARMEABI_TOOL_PREFIX="arm-linux-androideabi"
ARMEABI_COMMON_FLAGS="\
-isystem ${ANDROID_NDK}/platforms/android-8/arch-arm/usr/include \
-isystem ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/include \
-isystem ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi/include \
"
ARMEABI_CPP_STDLIB="${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi"

export ARCH="arm"
export CPP="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-cpp" \
export CXXCPP="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-cpp" \
export CPPFLAGS="--sysroot=${ARM_SYSROOT}" \
export CXXCPPFLAGS="--sysroot=${ARM_SYSROOT}" \
export CC="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-gcc" \
export CXX="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-g++" \
export LD="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-ld" \
export CXXFLAGS="-Os -s ${CC_ARMEABI_FLAGS} --sysroot=${ARM_SYSROOT} ${ARMEABI_COMMON_FLAGS}" \
export CFLAGS="-Os -s ${CC_ARMEABI_FLAGS} --sysroot=${ARM_SYSROOT} ${ARMEABI_COMMON_FLAGS}" \
export LDFLAGS="--sysroot=${ARM_SYSROOT} -L${ARMEABI_CPP_STDLIB} -lgnustl_static" \
export AR="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-ar" \
export AS="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-as" \
export LIBTOOL="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-libtool" \
export STRIP="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-strip" \
export RANLIB="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-ranlib" \

set +e
make -j2

# build for mips

# CC_MIPS_FLAGS="${CC_COMMON_FLAGS} \
# -fsigned-char \
# -fpic \
# -finline-functions \
# -funwind-tables \
# -fmessage-length=0 \
# -fno-inline-functions-called-once \
# -fgcse-after-reload \
# -frerun-cse-after-loop \
# -frename-registers"

# TODO if needed

echo
echo "Copy over libre.a files by yourself"
echo