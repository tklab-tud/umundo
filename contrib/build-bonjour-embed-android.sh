#!/bin/bash

# exit on error
set -e

ME=`basename $0`
TARGET_DEVICE="arm-linux-androideabi"
DEST_DIR="${PWD}/../../prebuilt/android/${TARGET_DEVICE}/lib"

if [ ! -d mDNSPosix ]; then
	echo
	echo "Cannot find mDNSPosix"
	echo "Run script from within mDNSResponder directory:"
	echo "mDNSResponder-333.10$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

. ../../find-android-ndk.sh

if [ -f ispatched ]; then
	rm ./ispatched
else
	patch -p1 < ../mDNSResponder-333.10.umundo.patch
fi

# #ifdef TARGET_OS_ANDROID
# 	__android_log_print(ANDROID_LOG_VERBOSE, "umundo", "%s:%d: mDNSPlatformTimeInit returned %d\n", __FILE__, __LINE__, result);
# #endif

# echo '
# #include <stdlib.h>
# #include "mDNSEmbeddedAPI.h"
# #define RR_CACHE_SIZE 1000
# static CacheEntity rrcachestorage[RR_CACHE_SIZE];
# mDNS mDNSStorage;
# struct mDNS_PlatformSupport_struct {};
# static mDNS_PlatformSupport platformSupport;
# static int mDNSisInitialized = 0;
# const char ProgramName[] = "umundo";
# 
# mDNSexport int embedded_mDNS_Init() {
# 	if (!mDNSisInitialized) {
# 		mStatus err = mDNS_Init(&mDNSStorage, &platformSupport, rrcachestorage, RR_CACHE_SIZE, mDNS_Init_DontAdvertiseLocalAddresses, mDNS_Init_NoInitCallback, mDNS_Init_NoInitCallbackContext);
# 		mDNSisInitialized = 1;
# 		return err;
# 	}
# }
# ' > dummy.c

touch ispatched

mDNSEmbedded=( mDNSShared/dnssd_clientshim.c mDNSPosix/mDNSPosix.c mDNSCore/mDNS.c mDNSCore/DNSCommon.c mDNSShared/mDNSDebug.c \
	mDNSShared/GenLinkedList.c mDNSCore/uDNS.c mDNSShared/PlatformCommon.c mDNSPosix/mDNSUNP.c mDNSCore/DNSDigest.c )

ANDROID_BIN_PREFIX=${ANDROID_NDK_ROOT}/toolchains/arm-linux-androideabi-4.4.3/prebuilt/darwin-x86/bin/arm-linux-androideabi
AR=${ANDROID_BIN_PREFIX}-ar
CC=${ANDROID_BIN_PREFIX}-gcc
LD=${ANDROID_BIN_PREFIX}-ld
SYSROOT=${ANDROID_NDK_ROOT}/platforms/android-14/arch-arm

CC_FLAGS="\
 -ImDNSCore -ImDNSShared -fwrapv -W -Wall -DPID_FILE=\"/var/run/mdnsd.pid\" \
 -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\" -DTARGET_OS_ANDROID -DNOT_HAVE_SA_LEN -DUSES_NETLINK -fpic\
 -Wdeclaration-after-statement -Os -llog"

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
		echo ${CC} --sysroot=${SYSROOT} ${CC_FLAGS} ${file} -o build/${file}.o
		${CC} --sysroot=${SYSROOT} -DMDNS_DEBUGMSGS=0 ${CC_FLAGS} -c ${file} -o build/${file}.o
		OBJS="$OBJS build/${file}.o"
	fi
done

echo ${CC} --sysroot=${SYSROOT} -shared -o build/prod/libmDNSEmbedded.so $OBJS
${CC} --sysroot=${SYSROOT} -shared -o build/prod/libmDNSEmbedded.so $OBJS
echo ${AR} rvs build/prod/libmDNSEmbedded.a $OBJS
${AR} rvs build/prod/libmDNSEmbedded.a $OBJS

# compile again with debugging
OBJS=""
for file in ${mDNSEmbedded[@]}; do
	if [ ! -f build/${file}_d.o ]; then
		echo ${CC} --sysroot=${SYSROOT} ${CC_FLAGS} ${file} -o build/${file}_d.o
		${CC} --sysroot=${SYSROOT} -g -DMDNS_DEBUGMSGS=5 ${CC_FLAGS} -c ${file} -o build/${file}_d.o
		OBJS="$OBJS build/${file}_d.o"
	fi
done

echo ${CC} --sysroot=${SYSROOT} -shared -o build/prod/libmDNSEmbedded_d.so $OBJS
${CC} --sysroot=${SYSROOT} -shared -o build/prod/libmDNSEmbedded_d.so $OBJS
echo ${AR} rvs build/prod/libmDNSEmbedded_d.a $OBJS
${AR} rvs build/prod/libmDNSEmbedded_d.a $OBJS

cp build/prod/* ${DEST_DIR}