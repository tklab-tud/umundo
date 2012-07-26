#!/bin/bash

#
# build ZeroMQ
#

# exit on error
set -e

ME=`basename $0`
DEST_DIR="${PWD}/../../prebuilt/linux-i686/gnu/lib"

if [ ! -f src/zmq.cpp ]; then
	echo
	echo "Cannot find src/zmq.cpp"
	echo "Run script from within zeroMQ directory:"
	echo "zeromq-3.1.0$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

if [ -f Makefile ]; then
	make clean
fi

CXXFLAGS="-s" ./configure \
--enable-static \
--enable-shared \
--prefix=${DEST_DIR} \

make -j2 install

# tidy up
rm -rf ${DEST_DIR}/share
rm -rf ${DEST_DIR}/lib/pkgconfig
mv ${DEST_DIR}/lib/* ${DEST_DIR}
rm -rf ${DEST_DIR}/lib
#mkdir -p ${DEST_DIR}/../../../include/zeromq
#mv ${DEST_DIR}/include/* ${DEST_DIR}/../../../include/zeromq
rm -rf ${DEST_DIR}/include