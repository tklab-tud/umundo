#!/bin/bash

# exit on error
set -e

ME=`basename $0`
ARCH=`uname -m`
DEST_DIR="${PWD}/../../prebuilt/linux-${ARCH}/gnu/lib"

if [ ! -d mDNSPosix ]; then
	echo
	echo "Cannot find mDNSPosix"
	echo "Run script from within mDNSResponder directory:"
	echo "mDNSResponder-333.10$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

if [ -f ispatched ]; then
	rm ./ispatched
else
	patch -p1 < ../mDNSResponder-333.10.android.patch
fi

touch ispatched

mDNSEmbedded=( mDNSShared/dnssd_clientshim.c mDNSPosix/mDNSPosix.c mDNSCore/mDNS.c mDNSCore/DNSCommon.c mDNSShared/mDNSDebug.c \
	mDNSShared/GenLinkedList.c mDNSCore/uDNS.c mDNSShared/PlatformCommon.c mDNSPosix/mDNSUNP.c mDNSCore/DNSDigest.c )

AR=ar
CC=gcc
LD=ld

CC_FLAGS="\
 -ImDNSCore -ImDNSShared -fwrapv -fno-strict-aliasing -W -Wall -DPID_FILE=\"/var/run/mdnsd.pid\" \
 -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\" -DTARGET_OS_LINUX -DNOT_HAVE_SA_LEN -DHAVE_LINUX -DUSES_NETLINK -fpic\
 -Wdeclaration-after-statement -Os -DMDNS_DEBUGMSGS=0"

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
		echo ${CC} ${CC_FLAGS} ${file} -o build/${file}.o
		${CC} ${CC_FLAGS} -c ${file} -o build/${file}.o
		OBJS="$OBJS build/${file}.o"
	fi
done

echo ${CC} -shared -o build/prod/libmDNSEmbedded.so $OBJS
${CC} -shared -o build/prod/libmDNSEmbedded.so $OBJS
echo ${AR} rvs build/prod/libmDNSEmbedded.a $OBJS
${AR} rvs build/prod/libmDNSEmbedded.a $OBJS

# compile again with debugging 
OBJS=""
for file in ${mDNSEmbedded[@]}; do
	if [ ! -f build/${file}_d.o ]; then
		echo ${CC} -g ${CC_FLAGS} ${file} -o build/${file}_d.o
		${CC} -g ${CC_FLAGS} -c ${file} -o build/${file}_d.o
		OBJS="$OBJS build/${file}_d.o"
	fi
done

echo ${CC} -g -shared -o build/prod/libmDNSEmbedded_d.so $OBJS
${CC} -g -shared -o build/prod/libmDNSEmbedded_d.so $OBJS
echo ${AR} rvs build/prod/libmDNSEmbedded_d.a $OBJS
${AR} rvs build/prod/libmDNSEmbedded_d.a $OBJS

cp build/prod/* ${DEST_DIR}
