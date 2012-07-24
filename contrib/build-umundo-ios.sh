#!/bin/bash

#
# build all of umundo for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
SDK_VER="5.1"
CWD=`pwd`
BUILD_DIR="/tmp/build-umundo-ios"

mkdir -p ${BUILD_DIR} &> /dev/null

if [[ -z $1 || $1 = "Debug" ]] ; then
mkdir -p ${BUILD_DIR}/iossim-debug &> /dev/null
cd ${BUILD_DIR}/iossim-debug
cmake ${DIR}/../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/cmake/CrossCompile-iOS-Sim.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Debug && make -j2

mkdir -p ${BUILD_DIR}/ios-debug &> /dev/null
cd ${BUILD_DIR}/ios-debug
cmake ${DIR}/../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/cmake/CrossCompile-iOS.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Debug && make -j2

# build universal libraries for debug
cd ${DIR}
lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundocore_d.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundocore_d.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundocore_d.a

lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundoserial_d.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundoserial_d.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundoserial_d.a

lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundorpc_d.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundorpc_d.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundorpc_d.a

lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundoutil_d.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundoutil_d.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundoutil_d.a

fi

if [[ -z $1 || $1 = "Release" ]] ; then

mkdir -p ${BUILD_DIR}/iossim-release &> /dev/null
cd ${BUILD_DIR}/iossim-release
cmake ${DIR}/../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/cmake/CrossCompile-iOS-Sim.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release && make -j2

mkdir -p ${BUILD_DIR}/ios-release &> /dev/null
cd ${BUILD_DIR}/ios-release
cmake ${DIR}/../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/cmake/CrossCompile-iOS.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release && make -j2

# build universal libraries for release
cd ${DIR}
lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundocore.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundocore.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundocore.a

lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundoserial.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundoserial.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundoserial.a

lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundorpc.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundorpc.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundorpc.a

lipo -create -output ../package/cross-compiled/ios-${SDK_VER}/libumundoutil.ios.a \
../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundoutil.a \
../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundoutil.a

fi


