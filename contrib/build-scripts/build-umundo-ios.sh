#!/bin/bash

#
# build all of umundo for iOS and iOS simulator
#

# make sure this is not set
unset MACOSX_DEPLOYMENT_TARGET

# be ridiculously conservative with regard to ios features
export IPHONEOS_DEPLOYMENT_TARGET="8.0"

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
SDK_VER="9.3"
CWD=`pwd`
BUILD_DIR="/tmp/build-umundo-ios"

#rm -rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR} &> /dev/null

#
# Build device with older SDK for old architectures
#
export IOS_SDK_VERSION=${SDK_VER}

mkdir -p ${BUILD_DIR}/device-${SDK_VER}-debug &> /dev/null
cd ${BUILD_DIR}/device-${SDK_VER}-debug
cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS.cmake \
-DDIST_PREPARE=ON \
-DCMAKE_BUILD_TYPE=Debug \
-DBUILD_CONVENIENCE_LIB=ON
make VERBOSE=1 -j2
mv ${DIR}/../../package/cross-compiled/ios/ \
   ${DIR}/../../package/cross-compiled/ios-device-${SDK_VER}-debug


mkdir -p ${BUILD_DIR}/device-${SDK_VER}-release &> /dev/null
cd ${BUILD_DIR}/device-${SDK_VER}-release
cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS.cmake \
-DDIST_PREPARE=ON \
-DCMAKE_BUILD_TYPE=Release \
-DBUILD_CONVENIENCE_LIB=ON
make VERBOSE=1 -j2
mv ${DIR}/../../package/cross-compiled/ios/ \
   ${DIR}/../../package/cross-compiled/ios-device-${SDK_VER}-release

mkdir -p ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-debug &> /dev/null
cd ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-debug
cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS-Sim.cmake \
-DDIST_PREPARE=ON \
-DCMAKE_BUILD_TYPE=Debug \
-DBUILD_CONVENIENCE_LIB=ON
make VERBOSE=1 -j2
mv ${DIR}/../../package/cross-compiled/ios/ \
   ${DIR}/../../package/cross-compiled/ios-simulator-${SDK_VER}-debug

mkdir -p ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-release &> /dev/null
cd ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-release
cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS-Sim.cmake \
-DDIST_PREPARE=ON \
-DCMAKE_BUILD_TYPE=Release \
-DBUILD_CONVENIENCE_LIB=ON
make VERBOSE=1 -j2
mv ${DIR}/../../package/cross-compiled/ios/ \
   ${DIR}/../../package/cross-compiled/ios-simulator-${SDK_VER}-release


#
# Build device with current SDK
#
# export IOS_SDK_VERSION=6.1
# 
# mkdir -p ${BUILD_DIR}/device-${IOS_SDK_VERSION}-debug &> /dev/null
# cd ${BUILD_DIR}/device-${IOS_SDK_VERSION}-debug
# cmake ${DIR}/../../ \
# -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS.cmake \
# -DDIST_PREPARE=ON \
# -DCMAKE_BUILD_TYPE=Debug \
# -DBUILD_CONVENIENCE_LIB=ON
# make -j2
# 
# mkdir -p ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-debug &> /dev/null
# cd ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-debug
# cmake ${DIR}/../../ \
# -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS-Sim.cmake \
# -DDIST_PREPARE=ON \
# -DCMAKE_BUILD_TYPE=Debug \
# -DBUILD_CONVENIENCE_LIB=ON
# make -j2
# 
# mkdir -p ${BUILD_DIR}/device-${IOS_SDK_VERSION}-release &> /dev/null
# cd ${BUILD_DIR}/device-${IOS_SDK_VERSION}-release
# cmake ${DIR}/../../ \
# -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS.cmake \
# -DDIST_PREPARE=ON \
# -DCMAKE_BUILD_TYPE=Release \
# -DBUILD_CONVENIENCE_LIB=ON
# make -j2
# 
# mkdir -p ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-release &> /dev/null
# cd ${BUILD_DIR}/simulator-${IOS_SDK_VERSION}-release
# cmake ${DIR}/../../ \
# -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-iOS-Sim.cmake \
# -DDIST_PREPARE=ON \
# -DCMAKE_BUILD_TYPE=Release \
# -DBUILD_CONVENIENCE_LIB=ON
# make -j2

#
# Create universal binary
#

LIBS=`find ${DIR}/../../package/cross-compiled/ios-* -name *.a`
set +e
for LIB in ${LIBS}; do
  LIB_BASE=`basename $LIB .a`
  ARCHS=`xcrun -sdk iphoneos lipo -info $LIB`
  ARCHS=`expr "$ARCHS" : '.*:\(.*\)$'`
  for ARCH in ${ARCHS}; do
    mkdir -p ${DIR}/../../package/cross-compiled/ios/arch/${ARCH} > /dev/null
    xcrun -sdk iphoneos lipo -extract $ARCH $LIB -output ${DIR}/../../package/cross-compiled/ios/arch/${ARCH}/${LIB_BASE}.a \
      || cp $LIB ${DIR}/../../package/cross-compiled/ios/arch/${ARCH}/${LIB_BASE}.a
    UNIQUE_LIBS=`ls ${DIR}/../../package/cross-compiled/ios/arch/${ARCH}`
  done
done

mkdir -p ${DIR}/../../package/cross-compiled/ios/lib &> /dev/null

for LIB in ${UNIQUE_LIBS}; do
  FILELIST=""
  for ARCH in `ls ${DIR}/../../package/cross-compiled/ios/arch/`; do
    FILELIST="${FILELIST} ${DIR}/../../package/cross-compiled/ios/arch/${ARCH}/${LIB}"
  done
  xcrun -sdk iphoneos lipo -create ${FILELIST} -output ${DIR}/../../package/cross-compiled/ios/lib/${LIB}
done

rm -rf ${DIR}/../../package/cross-compiled/ios/arch
rm -rf ${DIR}/../../package/cross-compiled/ios-*/

