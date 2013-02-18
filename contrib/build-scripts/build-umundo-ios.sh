#!/bin/bash

#
# build all of umundo for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
SDK_VER="6.1"
CWD=`pwd`
BUILD_DIR="/tmp/build-umundo-ios"

unset MACOSX_DEPLOYMENT_TARGET

rm -rf ${BUILD_DIR} && mkdir -p ${BUILD_DIR} &> /dev/null

if [[ -z $1 || $1 = "Debug" ]] ; then
mkdir -p ${BUILD_DIR}/iossim-debug &> /dev/null
cd ${BUILD_DIR}/iossim-debug
cmake ${DIR}/../../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS-Sim.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Debug && make -j2

mkdir -p ${BUILD_DIR}/ios-debug &> /dev/null
cd ${BUILD_DIR}/ios-debug
cmake ${DIR}/../../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Debug && make -j2

# build universal libraries for debug
cd ${DIR}

lipo -create -output ../../package/cross-compiled/ios-${SDK_VER}/libumundo_d.ios.a \
../../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundo_d.a \
../../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundo_d.a

fi

if [[ -z $1 || $1 = "Release" ]] ; then

mkdir -p ${BUILD_DIR}/iossim-release &> /dev/null
cd ${BUILD_DIR}/iossim-release
cmake ${DIR}/../../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS-Sim.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release && make -j2

mkdir -p ${BUILD_DIR}/ios-release &> /dev/null
cd ${BUILD_DIR}/ios-release
cmake ${DIR}/../../ -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release && make -j2

# build universal libraries for release
cd ${DIR}

lipo -create -output ../../package/cross-compiled/ios-${SDK_VER}/libumundo.ios.a \
../../package/cross-compiled/ios-${SDK_VER}/arm/lib/libumundo.a \
../../package/cross-compiled/ios-${SDK_VER}/i386/lib/libumundo.a

fi


