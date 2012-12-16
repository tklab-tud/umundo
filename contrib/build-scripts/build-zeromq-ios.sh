#!/bin/bash

#
# build ZeroMQ for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 
SDK_VER="6.0"
DEST_DIR="${DIR}/../prebuilt/ios/${SDK_VER}"
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

SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS${SDK_VER}.sdk"

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
CXX=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/g++ \
CC=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc \
LD=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ld\ -r \
CFLAGS="-Os -isysroot ${SYSROOT} -arch armv7 -arch armv7s -marm -miphoneos-version-min=4.3" \
CXXFLAGS="-Os -isysroot ${SYSROOT} -arch armv7 -arch armv7s -marm -miphoneos-version-min=4.3" \
--disable-dependency-tracking \
--host=arm-apple-darwin10 \
LDFLAGS="-isysroot ${SYSROOT} -arch armv7 -arch armv7s -miphoneos-version-min=4.3" \
AR=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ar \
AS=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/as \
LIBTOOL=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/libtool \
STRIP=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/strip \
RANLIB=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/ranlib \
--prefix=${BUILD_DIR}/ios

make -j2 install


#
# Simulator
#

SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator${SDK_VER}.sdk"

if [ -f Makefile ]; then
	make clean
fi

mkdir -p ${BUILD_DIR}/ios-sim &> /dev/null

make clean
./configure \
CXX=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/g++ \
CC=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/gcc \
LD=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ld\ -r \
CFLAGS="-Os -isysroot ${SYSROOT} -arch i386 -miphoneos-version-min=4.3" \
CXXFLAGS="-Os -isysroot ${SYSROOT} -arch i386 -miphoneos-version-min=4.3" \
--disable-dependency-tracking \
LDFLAGS="-isysroot  ${SYSROOT} -arch i386 -miphoneos-version-min=4.3" \
AR=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ar \
AS=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/as \
LIBTOOL=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/libtool \
STRIP=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/strip \
RANLIB=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/ranlib \
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
