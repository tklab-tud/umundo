#!/bin/bash

#
# build ZeroMQ for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 
MACOSX_VER=`/usr/bin/sw_vers -productVersion`
MACOSX_COMP=(`echo $MACOSX_VER | tr '.' ' '`)
DEST_DIR="${DIR}/../prebuilt/darwin-i386/${MACOSX_COMP[0]}.${MACOSX_COMP[1]}/gnu"
BUILD_DIR="/tmp/zeromq"


if [ ! -f src/zmq.cpp ]; then
	echo
	echo "Cannot find src/zmq.cpp"
	echo "Run script from within zeroMQ directory:"
	echo "zeromq-3.1.0$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR}/include &> /dev/null
mkdir -p ${DEST_DIR}/lib &> /dev/null
mkdir -p ${BUILD_DIR} &> /dev/null
mkdir -p ${BUILD_DIR}/x86_64 &> /dev/null
mkdir -p ${BUILD_DIR}/x86_64_d &> /dev/null
mkdir -p ${BUILD_DIR}/i386 &> /dev/null
mkdir -p ${BUILD_DIR}/i386_d &> /dev/null
mkdir -p ${DEST_DIR}/include &> /dev/null

if [ -f Makefile ]; then
	make clean
fi

# CFLAGS="-Os -mmacosx-version-min=10.6 -arch x86_64" \
# CXXFLAGS="-Os -mmacosx-version-min=10.6 -arch x86_64" \

if [ ${MACOSX_COMP[1]} -lt 9 ]; then
  MACOSX_VERSION_MIN="-mmacosx-version-min=10.6 -stdlib=libstdc++"
else
  MACOSX_VERSION_MIN="-mmacosx-version-min=10.7 -stdlib=libc++"
fi

# 64Bit release
./configure \
CFLAGS="-Os ${MACOSX_VERSION_MIN} -arch x86_64" \
CXXFLAGS="-Os ${MACOSX_VERSION_MIN} -arch x86_64" \
LDFLAGS="${MACOSX_VERSION_MIN} -arch x86_64" \
--enable-static \
--prefix=${BUILD_DIR}/x86_64

make -j2 install
make clean

cp ${BUILD_DIR}/x86_64/include/* ${DEST_DIR}/include

# 64Bit debug
./configure \
CFLAGS="-g ${MACOSX_VERSION_MIN} -arch x86_64" \
CXXFLAGS="-g ${MACOSX_VERSION_MIN} -arch x86_64" \
LDFLAGS="${MACOSX_VERSION_MIN} -arch x86_64" \
--enable-static \
--prefix=${BUILD_DIR}/x86_64_d

make -j2 install
make clean

# 32Bit release
./configure \
CFLAGS="-Os ${MACOSX_VERSION_MIN} -arch i386" \
CXXFLAGS="-Os ${MACOSX_VERSION_MIN} -arch i386" \
LDFLAGS="${MACOSX_VERSION_MIN} -arch i386" \
--enable-static \
--prefix=${BUILD_DIR}/i386

make -j2 install
make clean

# 32Bit debug
./configure \
CFLAGS="-g ${MACOSX_VERSION_MIN} -arch i386" \
CXXFLAGS="-g ${MACOSX_VERSION_MIN} -arch i386" \
LDFLAGS="${MACOSX_VERSION_MIN} -arch i386" \
--enable-static \
--prefix=${BUILD_DIR}/i386_d

make -j2 install
make clean

#
# create universal library
#
lipo -create ${BUILD_DIR}/x86_64/lib/libzmq.a ${BUILD_DIR}/i386/lib/libzmq.a -output ${DEST_DIR}/lib/libzmq.a
echo "Built universal library in: ${DEST_DIR}/lib/libzmq.a"
lipo -info ${DEST_DIR}/lib/libzmq.a

lipo -create ${BUILD_DIR}/x86_64_d/lib/libzmq.a ${BUILD_DIR}/i386_d/lib/libzmq.a -output ${DEST_DIR}/lib/libzmq_d.a
echo "Built universal library in: ${DEST_DIR}/lib/libzmq_d.a"
lipo -info ${DEST_DIR}/lib/libzmq_d.a
