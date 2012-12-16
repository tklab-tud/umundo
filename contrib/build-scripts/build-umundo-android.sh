#!/bin/bash

#
# build all of umundo for android
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`
BUILD_DIR="/tmp/build-umundo-android"

#. ${DIR}/find-android-ndk.sh

mkdir -p ${BUILD_DIR} &> /dev/null

if [[ -z $1 || $1 = "Debug" ]] ; then
cd ${BUILD_DIR}
rm -rf * && cmake ${DIR}/../../ -DANDROID_ABI=armeabi -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Debug && make -j2 && make -j2 java
fi

if [[ -z $1 || $1 = "Release" ]] ; then
cd ${BUILD_DIR}
rm -rf * && cmake ${DIR}/../../ -DANDROID_ABI=armeabi -DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release && make -j2 && make -j2 java
fi


