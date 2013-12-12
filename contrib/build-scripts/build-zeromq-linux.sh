#!/bin/bash

#
# build ZeroMQ
#

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 
CPUARCH=`uname -m`
DEST_DIR="${DIR}/../prebuilt/linux-${CPUARCH}/gnu"
BUILD_DIR="/tmp/zeromq"

if [ ! -f src/zmq.cpp ]; then
	echo
	echo "Cannot find src/zmq.cpp"
	echo "Run script from within zeroMQ directory:"
	echo "zeromq-3.1.0$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR}/lib &> /dev/null

if [ -f Makefile ]; then
	make clean
fi

./configure \
CXXFLAGS="-s -fPIC" \
--prefix=${BUILD_DIR} \
--enable-static

mkdir -p ${DEST_DIR}/include 2>&1 > /dev/null

make -j2
make install
cp ${BUILD_DIR}/lib/libzmq.a ${DEST_DIR}/lib/libzmq.a
cp ${BUILD_DIR}/include/* ${DEST_DIR}/include

make clean
./configure \
CXXFLAGS="-g -fPIC" \
--prefix=${BUILD_DIR} \
--enable-static

make -j2
make install
cp ${BUILD_DIR}/lib/libzmq.a ${DEST_DIR}/lib/libzmq_d.a
