#!/bin/bash

#
# build PCRE for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
SDK_VER="5.0"
DEST_DIR="${PWD}/../../prebuilt/ios/${SDK_VER}"

if [ ! -f pcre_version.c ]; then
	echo
	echo "Cannot find pcre_version.c"
	echo "Run script from within pcre directory:"
	echo "pcre-8.31$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

#
# Build for Device
#

SYSROOT="/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${SDK_VER}.sdk"

if [ ! -d ${SYSROOT} ]; then
	echo
	echo "Cannot find iOS developer tools."
	echo
	exit	
fi

if [ -f Makefile ]; then
	make clean
fi

mkdir -p ${DEST_DIR}/device &> /dev/null

./configure \
--disable-shared \
--enable-utf8 \
CPP="cpp" \
CXXCPP="cpp" \
CXX=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/g++ \
CC=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc \
LD=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ld\ -r \
CFLAGS="-O -isysroot ${SYSROOT} -arch armv7 -arch armv6" \
CXXFLAGS="-O -isysroot ${SYSROOT} -arch armv7 -arch armv6" \
--disable-dependency-tracking \
--host=arm-apple-darwin10 \
--target=arm-apple-darwin10 \
LDFLAGS="-isysroot ${SYSROOT} -arch armv7 -arch armv6" \
AR=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ar \
AS=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/as \
LIBTOOL=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/libtool \
STRIP=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/strip \
RANLIB=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ranlib \
--prefix=${DEST_DIR}/device

make -j2 install


#
# Simulator
#

SYSROOT="/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator${SDK_VER}.sdk"

if [ -f Makefile ]; then
	make clean
fi

mkdir -p ${DEST_DIR}/simulator &> /dev/null

make clean
./configure \
--disable-shared \
--enable-utf8 \
CXX=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/g++ \
CC=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/gcc \
LD=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ld\ -r \
CFLAGS="-O -isysroot ${SYSROOT} -arch i386" \
CXXFLAGS="-O -isysroot ${SYSROOT} -arch i386" \
--disable-dependency-tracking \
LDFLAGS="-isysroot  ${SYSROOT} -arch i386" \
AR=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ar \
AS=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/as \
LIBTOOL=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/libtool \
STRIP=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/strip \
RANLIB=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ranlib \
--prefix=${DEST_DIR}/simulator

make -j2 install

# tidy up
rm -rf ${DEST_DIR}/simulator/bin
rm -rf ${DEST_DIR}/simulator/include
rm -rf ${DEST_DIR}/simulator/share
rm -rf ${DEST_DIR}/simulator/lib/pkgconfig
rm -rf ${DEST_DIR}/simulator/lib/*.la

rm -rf ${DEST_DIR}/device/bin
rm -rf ${DEST_DIR}/device/include
rm -rf ${DEST_DIR}/device/share
rm -rf ${DEST_DIR}/device/lib/pkgconfig
rm -rf ${DEST_DIR}/device/lib/*.la

echo
echo "- Finished Build --------------------------------------"
echo
#
# create universal library
#
lipo -create ${DEST_DIR}/simulator/lib/libpcre.a ${DEST_DIR}/device/lib/libpcre.a -output ${DEST_DIR}/lib/libpcre.a
lipo -info ${DEST_DIR}/lib/libpcre.a

lipo -create ${DEST_DIR}/simulator/lib/libpcrecpp.a ${DEST_DIR}/device/lib/libpcrecpp.a -output ${DEST_DIR}/lib/libpcrecpp.a
lipo -info ${DEST_DIR}/lib/libpcrecpp.a

lipo -create ${DEST_DIR}/simulator/lib/libpcreposix.a ${DEST_DIR}/device/lib/libpcreposix.a -output ${DEST_DIR}/lib/libpcreposix.a
lipo -info ${DEST_DIR}/lib/libpcreposix.a
