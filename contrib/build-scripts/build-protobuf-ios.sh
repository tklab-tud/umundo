#!/bin/bash

#
# build ProtoBuf for iOS and iOS simulator
#

PROTOC=`which protoc`

# exit on error
set -e

ME=`basename $0`
SDK_VER="5.0"
DEST_DIR="${PWD}/../../../prebuilt/protobuf/ios/${SDK_VER}"

if [ ! -d src/google/protobuf ]; then
	echo
	echo "Cannot find src/google/protobuf"
	echo "Run script from within protobuf directory:"
	echo "protobuf-2.4.1$ ../../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

if [ ! -n "${PROTOC}" ]; then
	echo
	echo "Cannot find native protoc compiler"
	echo "make sure 'which protoc' returns a path"
  echo
	exit
fi

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

mkdir -p ${DEST_DIR}/ios &> /dev/null

./configure \
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
--enable-shared=no \
--with-protoc=${PROTOC} \
LDFLAGS="-isysroot ${SYSROOT} -arch armv7 -arch armv6" \
AR=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ar \
AS=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/as \
LIBTOOL=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/libtool \
STRIP=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/strip \
RANLIB=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ranlib \
--prefix=${DEST_DIR}/ios

make -j2 install


#
# Simulator
#

SYSROOT="/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator${SDK_VER}.sdk"

if [ -f Makefile ]; then
	make clean
fi

mkdir -p ${DEST_DIR}/ios-sim &> /dev/null

make clean
./configure \
CXX=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/g++ \
CC=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/gcc \
LD=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ld\ -r \
CFLAGS="-O -isysroot ${SYSROOT} -arch i386" \
CXXFLAGS="-O -isysroot ${SYSROOT} -arch i386" \
--disable-dependency-tracking \
--enable-shared=no \
--with-protoc=${PROTOC} \
LDFLAGS="-isysroot  ${SYSROOT} -arch i386" \
AR=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ar \
AS=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/as \
LIBTOOL=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/libtool \
STRIP=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/strip \
RANLIB=/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ranlib \
--prefix=${DEST_DIR}/ios-sim

make -j2 install

# # tidy up
rm -rf ${DEST_DIR}/ios/bin
rm -rf ${DEST_DIR}/ios/include
rm -rf ${DEST_DIR}/ios/lib/pkgconfig
mv ${DEST_DIR}/ios/lib/* ${DEST_DIR}/ios
rm -rf ${DEST_DIR}/ios/lib
 
rm -rf ${DEST_DIR}/ios-sim/bin
rm -rf ${DEST_DIR}/ios-sim/include
rm -rf ${DEST_DIR}/ios-sim/lib/pkgconfig
mv ${DEST_DIR}/ios-sim/lib/* ${DEST_DIR}/ios-sim
rm -rf ${DEST_DIR}/ios-sim/lib

echo
echo "- Finished Build --------------------------------------"
echo
#
# create universal library
#
lipo -create ${DEST_DIR}/ios-sim/libprotobuf-lite.a ${DEST_DIR}/ios/libprotobuf-lite.a -output ${DEST_DIR}/libprotobuf-lite.a
lipo -info ${DEST_DIR}/libprotobuf-lite.a

lipo -create ${DEST_DIR}/ios-sim/libprotobuf.a ${DEST_DIR}/ios/libprotobuf.a -output ${DEST_DIR}/libprotobuf.a
lipo -info ${DEST_DIR}/libprotobuf.a

lipo -create ${DEST_DIR}/ios-sim/libprotoc.a ${DEST_DIR}/ios/libprotoc.a -output ${DEST_DIR}/libprotoc.a
lipo -info ${DEST_DIR}/libprotoc.a
