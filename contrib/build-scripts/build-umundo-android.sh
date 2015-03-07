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

# rm -rf ${BUILD_DIR} && 
mkdir -p ${BUILD_DIR} &> /dev/null
cd ${BUILD_DIR}


if [ ! -d "${ANDROID_NDK}" ]; then
  echo
  echo No Android NDK at ${ANDROID_NDK}
  echo export ANDROID_NDK as the NDK root
  echo
  exit
fi
echo
echo Using Android NDK at ${ANDROID_NDK}
echo


mkdir -p ${BUILD_DIR}/armeabi-debug &> /dev/null
cd ${BUILD_DIR}/armeabi-debug

cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake \
-DDIST_PREPARE=ON \
-DBUILD_UMUNDO_TOOLS=OFF \
-DBUILD_UMUNDO_S11N=OFF \
-DBUILD_UMUNDO_RPC=OFF \
-DBUILD_UMUNDO_UTIL=OFF \
-DBUILD_BINDINGS=ON \
-DBUILD_SHARED_LIBS=OFF \
-DBUILD_PREFER_STATIC_LIBRARIES=ON \
-DANDROID_ABI="armeabi" \
-DCMAKE_BUILD_TYPE=Debug
make -j4
make java


mkdir -p ${BUILD_DIR}/armeabi-release &> /dev/null
cd ${BUILD_DIR}/armeabi-release

cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake \
-DDIST_PREPARE=ON \
-DBUILD_UMUNDO_TOOLS=OFF \
-DBUILD_UMUNDO_S11N=OFF \
-DBUILD_UMUNDO_RPC=OFF \
-DBUILD_UMUNDO_UTIL=OFF \
-DBUILD_BINDINGS=ON \
-DBUILD_SHARED_LIBS=OFF \
-DBUILD_PREFER_STATIC_LIBRARIES=ON \
-DANDROID_ABI="armeabi" \
-DCMAKE_BUILD_TYPE=Release
make -j4
make java

mkdir -p ${BUILD_DIR}/x86-debug &> /dev/null
cd ${BUILD_DIR}/x86-debug

cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake \
-DDIST_PREPARE=ON \
-DBUILD_UMUNDO_TOOLS=OFF \
-DBUILD_UMUNDO_S11N=OFF \
-DBUILD_UMUNDO_RPC=OFF \
-DBUILD_UMUNDO_UTIL=OFF \
-DBUILD_BINDINGS=ON \
-DBUILD_SHARED_LIBS=OFF \
-DBUILD_PREFER_STATIC_LIBRARIES=ON \
-DANDROID_ABI="x86" \
-DCMAKE_BUILD_TYPE=Debug
make -j4
make java


mkdir -p ${BUILD_DIR}/x86-release &> /dev/null
cd ${BUILD_DIR}/x86-release

cmake ${DIR}/../../ \
-DCMAKE_TOOLCHAIN_FILE=${DIR}/../cmake/CrossCompile-Android.cmake \
-DDIST_PREPARE=ON \
-DBUILD_UMUNDO_TOOLS=OFF \
-DBUILD_UMUNDO_S11N=OFF \
-DBUILD_UMUNDO_RPC=OFF \
-DBUILD_UMUNDO_UTIL=OFF \
-DBUILD_BINDINGS=ON \
-DBUILD_SHARED_LIBS=OFF \
-DBUILD_PREFER_STATIC_LIBRARIES=ON \
-DANDROID_ABI="x86" \
-DCMAKE_BUILD_TYPE=Release
make -j4
make java

# create jar for recent android studio
mkdir -p ${DIR}/../../package/cross-compiled/android/android-studio/lib/armeabi
mkdir -p ${DIR}/../../package/cross-compiled/android/android-studio/lib/x86

cp ${DIR}/../../package/cross-compiled/android/armeabi/lib/libumundoNativeJava*.so \
   ${DIR}/../../package/cross-compiled/android/android-studio/lib/armeabi

cp ${DIR}/../../package/cross-compiled/android/x86/lib/libumundoNativeJava*.so \
   ${DIR}/../../package/cross-compiled/android/android-studio/lib/x86

cd ${DIR}/../../package/cross-compiled/android/android-studio

zip -qr umundoNative.zip lib
mv umundoNative.zip umundoNative.jar
rm -rf lib

CANON_DIR="`cd "${DIR}/../../package/cross-compiled/android";pwd`"


printf "Created android-studio/umundoNative.jar to use in Android Studio at:\n\t${CANON_DIR}/android-studio/umundoNative.jar\n\n"
cat << EOF
Respective build.cradle will have to contain something like:
	dependencies {
	    compile files('libs/android-support-v4.jar')
	    compile files('libs/umundo.jar')
	    compile fileTree(dir: 'libs', include: 'umundoNative.jar')
	}
EOF

cd ..
find . -name "*.a" -exec rm {} \;

# copy into umundo-pingpong example
# cp ${DIR}/../../package/cross-compiled/android/umundo.jar \
#    ${DIR}/../../examples/android/umundo-pingpong/libs/
# cp ${DIR}/../../package/cross-compiled/android/armv5te/lib/libumundoNativeJava_d.so \
#    ${DIR}/../../examples/android/umundo-pingpong/libs/armeabi/
# cp ${DIR}/../../package/cross-compiled/android/armv5te/lib/libumundoNativeJava.so \
#    ${DIR}/../../examples/android/umundo-pingpong/libs/armeabi/
# cp ${DIR}/../../package/cross-compiled/android/i686/lib/libumundoNativeJava_d.so \
#    ${DIR}/../../examples/android/umundo-pingpong/libs/x86/
# cp ${DIR}/../../package/cross-compiled/android/i686/lib/libumundoNativeJava.so \
#    ${DIR}/../../examples/android/umundo-pingpong/libs/x86/
