#!/bin/bash

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 
BUILD_DIR="/tmp/build-bonjour-android/"
DEST_DIR="${DIR}/../prebuilt/android"

if [ ! -d mDNSPosix ]; then
	echo
	echo "Cannot find mDNSPosix"
	echo "Run script from within mDNSResponder directory:"
	echo "mDNSResponder-333.10$ ../../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

if [ -f ispatched ]; then
	rm ./ispatched
else
	patch -p1 < ../mDNSResponder-333.10.umundo.patch
fi

# #ifdef TARGET_OS_ANDROID
# 	__android_log_print(ANDROID_LOG_VERBOSE, "umundo", "%s:%d: mDNSPlatformTimeInit returned %d\n", __FILE__, __LINE__, result);
# #endif

touch ispatched

mDNSEmbedded=( mDNSShared/dnssd_clientshim.c mDNSPosix/mDNSPosix.c mDNSCore/mDNS.c mDNSCore/DNSCommon.c mDNSShared/mDNSDebug.c \
	mDNSShared/GenLinkedList.c mDNSCore/uDNS.c mDNSShared/PlatformCommon.c mDNSPosix/mDNSUNP.c mDNSCore/DNSDigest.c )

CC_COMMON_FLAGS="-I. -DANDROID -DTARGET_OS_ANDROID -fno-strict-aliasing -fno-omit-frame-pointer -fno-exceptions -fdata-sections -ffunction-sections"
CC_COMMON_FLAGS="\
 ${CC_COMMON_FLAGS} \
 -ImDNSCore -ImDNSShared -fwrapv -W -Wall -DPID_FILE=\"/var/run/mdnsd.pid\" \
 -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\" -DTARGET_OS_ANDROID -DNOT_HAVE_SA_LEN -DUSES_NETLINK -fpic\
 -Wdeclaration-after-statement -Wno-unused-parameter -Wno-unused-but-set-variable -Os -llog"

# build for x86

CC_X86_FLAGS="${CC_COMMON_FLAGS} \
-funwind-tables \
-finline-limit=300"

X86_SYSROOT="${ANDROID_NDK}/platforms/android-9/arch-x86"
X86_TOOLCHAIN_ROOT="${ANDROID_NDK}/toolchains/x86-4.7/prebuilt/darwin-x86_64"
X86_TOOL_PREFIX="i686-linux-android"
X86_COMMON_FLAGS="\
-isystem ${ANDROID_NDK}/platforms/android-9/arch-x86/usr/include \
-isystem ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/include \
-isystem ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/libs/x86/include"

AR="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-ar"
CC="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-gcc"
LD="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-ld"
CPP="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-cpp" \
CXXCPP="${X86_TOOLCHAIN_ROOT}/bin/${X86_TOOL_PREFIX}-cpp" \
CPPFLAGS="--sysroot=${X86_SYSROOT}" \
CXXCPPFLAGS="--sysroot=${X86_SYSROOT}" \

if [ -d build ]; then
	rm -rf build
fi

mkdir -p build/mDNSPosix &> /dev/null
mkdir -p build/mDNSCore &> /dev/null
mkdir -p build/mDNSShared &> /dev/null
mkdir -p build/prod &> /dev/null

# compile all the files
OBJS=""
for file in ${mDNSEmbedded[@]}; do
	if [ ! -f build/${file}.o ]; then
		echo ${CC} --sysroot=${X86_SYSROOT} ${CC_X86_FLAGS} ${X86_COMMON_FLAGS} -c ${file} -o build/${file}.o
		${CC} --sysroot=${X86_SYSROOT} ${CC_X86_FLAGS} ${X86_COMMON_FLAGS} -c ${file} -o build/${file}.o
		OBJS="$OBJS build/${file}.o"
	fi
done

# echo ${CC} --sysroot=${X86_SYSROOT} -shared -o build/prod/libmDNSEmbedded.so $OBJS
# ${CC} --sysroot=${X86_SYSROOT} -shared -o build/prod/libmDNSEmbedded.so $OBJS
echo ${AR} rvs build/prod/libmDNSEmbedded.a $OBJS
${AR} rvs build/prod/libmDNSEmbedded.a $OBJS

mkdir -p ${DEST_DIR}/x86/include/bonjour &> /dev/null
mkdir -p ${DEST_DIR}/x86/lib &> /dev/null
cp build/prod/*.a ${DEST_DIR}/x86/lib
cp mDNSShared/CommonServices.h ${DEST_DIR}/x86/include/bonjour
cp mDNSShared/dns_sd.h ${DEST_DIR}/x86/include/bonjour
cp mDNSCore/DNSCommon.h ${DEST_DIR}/x86/include/bonjour
cp mDNSCore/mDNSDebug.h ${DEST_DIR}/x86/include/bonjour
cp mDNSCore/mDNSEmbeddedAPI.h ${DEST_DIR}/x86/include/bonjour
cp mDNSPosix/mDNSPosix.h ${DEST_DIR}/x86/include/bonjour
cp mDNSCore/uDNS.h ${DEST_DIR}/x86/include/bonjour

# build for arm

CC_ARMEABI_FLAGS="${CC_COMMON_FLAGS} \
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
-isystem ${ANDROID_NDK}/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi/include"

AR="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-ar"
CC="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-gcc"
LD="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-ld"
CPP="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-cpp" \
CXXCPP="${ARMEABI_TOOLCHAIN_ROOT}/bin/${ARMEABI_TOOL_PREFIX}-cpp" \
CPPFLAGS="--sysroot=${ARM_SYSROOT}" \
CXXCPPFLAGS="--sysroot=${ARM_SYSROOT}" \

if [ -d build ]; then
	rm -rf build
fi

mkdir -p build/mDNSPosix &> /dev/null
mkdir -p build/mDNSCore &> /dev/null
mkdir -p build/mDNSShared &> /dev/null
mkdir -p build/prod &> /dev/null

# compile all the files
OBJS=""
for file in ${mDNSEmbedded[@]}; do
	if [ ! -f build/${file}.o ]; then
		echo ${CC} --sysroot=${X86_SYSROOT} ${CC_ARMEABI_FLAGS} ${ARMEABI_COMMON_FLAGS} -c ${file} -o build/${file}.o
		${CC} --sysroot=${X86_SYSROOT} ${CC_ARMEABI_FLAGS} ${ARMEABI_COMMON_FLAGS} -c ${file} -o build/${file}.o
		OBJS="$OBJS build/${file}.o"
	fi
done

# echo ${CC} --sysroot=${X86_SYSROOT} -shared -o build/prod/libmDNSEmbedded.so $OBJS
# ${CC} --sysroot=${X86_SYSROOT} -shared -o build/prod/libmDNSEmbedded.so $OBJS
echo ${AR} rvs build/prod/libmDNSEmbedded.a $OBJS
${AR} rvs build/prod/libmDNSEmbedded.a $OBJS

mkdir -p ${DEST_DIR}/armeabi/include/bonjour &> /dev/null
mkdir -p ${DEST_DIR}/armeabi/lib &> /dev/null
cp build/prod/*.a ${DEST_DIR}/armeabi/lib
cp mDNSShared/CommonServices.h ${DEST_DIR}/armeabi/include/bonjour
cp mDNSShared/dns_sd.h ${DEST_DIR}/armeabi/include/bonjour
cp mDNSCore/DNSCommon.h ${DEST_DIR}/armeabi/include/bonjour
cp mDNSCore/mDNSDebug.h ${DEST_DIR}/armeabi/include/bonjour
cp mDNSCore/mDNSEmbeddedAPI.h ${DEST_DIR}/armeabi/include/bonjour
cp mDNSPosix/mDNSPosix.h ${DEST_DIR}/armeabi/include/bonjour
cp mDNSCore/uDNS.h ${DEST_DIR}/armeabi/include/bonjour
