#!/bin/bash

#
# build ZeroMQ for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 
DEST_DIR="${DIR}/../prebuilt/darwin-i386/gnu/lib"
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
mkdir -p ${DEST_DIR}/x86_64 &> /dev/null
mkdir -p ${DEST_DIR}/x86_64_d &> /dev/null
mkdir -p ${DEST_DIR}/i386 &> /dev/null
mkdir -p ${DEST_DIR}/i386_d &> /dev/null

if [ -f Makefile ]; then
	make clean
fi

# 64Bit release
./configure \
CFLAGS="-Os -mmacosx-version-min=10.6 -arch x86_64" \
CXXFLAGS="-Os -mmacosx-version-min=10.6 -arch x86_64" \
--enable-static --disable-eventfd \
LDFLAGS="-mmacosx-version-min=10.6 -arch x86_64" \
--prefix=${BUILD_DIR}/x86_64

make -j2 install
make clean

# 64Bit debug
./configure \
CFLAGS="-g -mmacosx-version-min=10.6 -arch x86_64" \
CXXFLAGS="-g -mmacosx-version-min=10.6 -arch x86_64" \
--enable-static --disable-eventfd \
LDFLAGS="-mmacosx-version-min=10.6 -arch x86_64" \
--prefix=${BUILD_DIR}/x86_64_d

make -j2 install
make clean

# 32Bit release
./configure \
CFLAGS="-Os -mmacosx-version-min=10.6 -arch i386" \
CXXFLAGS="-Os -mmacosx-version-min=10.6 -arch i386" \
--enable-static --disable-eventfd \
LDFLAGS="-mmacosx-version-min=10.6 -arch i386" \
--prefix=${BUILD_DIR}/i386

make -j2 install
make clean

# 32Bit debug
./configure \
CFLAGS="-g -mmacosx-version-min=10.6 -arch i386" \
CXXFLAGS="-g -mmacosx-version-min=10.6 -arch i386" \
--enable-static --disable-eventfd \
LDFLAGS="-mmacosx-version-min=10.6 -arch i386" \
--prefix=${BUILD_DIR}/i386_d

make -j2 install
make clean


#
# create universal library
#
lipo -create ${BUILD_DIR}/x86_64/lib/libzmq.a ${BUILD_DIR}/i386/lib/libzmq.a -output ${DEST_DIR}/libzmq.a
echo "Built universal library in: ${DEST_DIR}/libzmq.a"
lipo -info ${DEST_DIR}/libzmq.a

lipo -create ${BUILD_DIR}/x86_64_d/lib/libzmq.a ${BUILD_DIR}/i386_d/lib/libzmq.a -output ${DEST_DIR}/libzmq_d.a
echo "Built universal library in: ${DEST_DIR}/libzmq_d.a"
lipo -info ${DEST_DIR}/libzmq_d.a
