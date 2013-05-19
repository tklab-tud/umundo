#!/bin/bash

#
# build all of umundo for android
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`
BUILD_DIR="/tmp/build-umundo-ios"

rm -rf ${BUILD_DIR} && mkdir -p ${BUILD_DIR} &> /dev/null
cd ${BUILD_DIR}

#. ${DIR}/find-android-ndk.sh

#    ANDROID_ABI=armeabi-v7a -  specifies the target Application Binary
#      Interface (ABI). This option nearly matches to the APP_ABI variable
#      used by ndk-build tool from Android NDK.
#
#      Possible targets are:
#        "armeabi" - matches to the NDK ABI with the same name.
#           See ${ANDROID_NDK}/docs/CPU-ARCH-ABIS.html for the documentation.
#        "armeabi-v7a" - matches to the NDK ABI with the same name.
#           See ${ANDROID_NDK}/docs/CPU-ARCH-ABIS.html for the documentation.
#        "armeabi-v7a with NEON" - same as armeabi-v7a, but
#            sets NEON as floating-point unit
#        "armeabi-v7a with VFPV3" - same as armeabi-v7a, but
#            sets VFPV3 as floating-point unit (has 32 registers instead of 16).
#        "armeabi-v6 with VFP" - tuned for ARMv6 processors having VFP.
#        "x86" - matches to the NDK ABI with the same name.
#            See ${ANDROID_NDK}/docs/CPU-ARCH-ABIS.html for the documentation.
#        "mips" - matches to the NDK ABI with the same name
#            (not testes on real devices)

mkdir -p ${BUILD_DIR}/armeabi-release &> /dev/null
cd ${BUILD_DIR}/armeabi-release

cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake \
-DDIST_PREPARE=ON \
-DANDROID_ABI="armeabi" \
-DCMAKE_BUILD_TYPE=Release
make -j2 VERBOSE=1
make -j2 java

mkdir -p ${BUILD_DIR}/x86-release &> /dev/null
cd ${BUILD_DIR}/x86-release

cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake \
-DDIST_PREPARE=ON \
-DANDROID_ABI="x86" \
-DCMAKE_BUILD_TYPE=Release
make -j2 VERBOSE=1
make -j2 java