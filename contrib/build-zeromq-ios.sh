#!/bin/bash

#
# build ZeroMQ for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
SDK_VER="5.0"
DEST_DIR="${PWD}/../../prebuilt/ios/${SDK_VER}"
BUILD_DIR="/tmp/zeromq"

if [ ! -f src/zmq.cpp ]; then
	echo
	echo "Cannot find src/zmq.cpp"
	echo "Run script from within zeroMQ directory:"
	echo "zeromq-3.1.0$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null
mkdir -p ${DEST_DIR}/device/lib &> /dev/null
mkdir -p ${DEST_DIR}/simulator/lib &> /dev/null

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

mkdir -p ${BUILD_DIR}/ios &> /dev/null

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
LDFLAGS="-isysroot ${SYSROOT} -arch armv7 -arch armv6" \
AR=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ar \
AS=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/as \
LIBTOOL=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/libtool \
STRIP=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/strip \
RANLIB=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ranlib \
--prefix=${BUILD_DIR}/ios

make -j2 install


#
# Simulator
#

SYSROOT="/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator${SDK_VER}.sdk"

if [ -f Makefile ]; then
	make clean
fi

mkdir -p ${BUILD_DIR}/ios-sim &> /dev/null

make clean
./configure \
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
--prefix=${BUILD_DIR}/ios-sim

make -j2 install

# tidy up
mv ${BUILD_DIR}/ios/lib/lib* ${DEST_DIR}/device/lib
mv ${BUILD_DIR}/ios-sim/lib/lib* ${DEST_DIR}/simulator/lib

#
# create universal library
#
lipo -info ${DEST_DIR}/device/lib/libzmq.a
lipo -info ${DEST_DIR}/simulator/lib/libzmq.a
lipo -create ${DEST_DIR}/device/lib/libzmq.a ${DEST_DIR}/simulator/lib/libzmq.a -output ${DEST_DIR}/lib/libzmq.a
echo "Built universal library in: ${DEST_DIR}/lib/libzmq.a"
lipo -info ${DEST_DIR}/lib/libzmq.a
