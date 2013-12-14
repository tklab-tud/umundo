#!/bin/bash

#
# build PCRE for iOS and iOS simulator
#

# make sure this is not set
unset MACOSX_DEPLOYMENT_TARGET

# be ridiculously conservative with regard to ios features
export IPHONEOS_DEPLOYMENT_TARGET="4.3"

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
SDK_VER="7.0"
#SDK_VER="5.1"
DEST_DIR="${DIR}/../prebuilt/ios/${SDK_VER}-pcre-build"

if [ ! -f pcre_version.c ]; then
	echo
	echo "Cannot find pcre_version.c"
	echo "Run script from within pcre directory:"
	echo "pcre-8.31$ ../../../${ME}"
	echo
	exit
fi

mkdir -p ${DEST_DIR} &> /dev/null

# see http://stackoverflow.com/questions/2424770/floating-point-comparison-in-shell-script
if [ $(bc <<< "$SDK_VER >= 7.0") -eq 1 ]; then
  DEV_ARCHS="-arch armv7 -arch armv7s -arch arm64"
elif [ $(bc <<< "$SDK_VER >= 6.1") -eq 1 ]; then
  DEV_ARCHS="-arch armv7 -arch armv7s"
elif [ $(bc <<< "$SDK_VER >= 5.1") -eq 1 ]; then
  DEV_ARCHS="-arch armv6 -arch armv7"
else
  echo
  echo "Building for SDK < 5.1 not supported"
  exit
fi

#
# Build for Device
#
if [ ! -d ${DEST_DIR}/device ]; then

  TOOLCHAIN_ROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer" 
  SYSROOT="${TOOLCHAIN_ROOT}/SDKs/iPhoneOS${SDK_VER}.sdk"

  if [ $(bc <<< "$SDK_VER >= 7.0") -eq 1 ]; then
    
    # export CC="/Applications/Xcode.app/Contents/Developer/usr/bin/gcc"
    # export CXX="/Applications/Xcode.app/Contents/Developer/usr/bin/g++"
    # export CPPFLAGS="-sysroot ${SYSROOT}"
    # export CXXCPPFLAGS="-sysroot ${SYSROOT}"
    # export LD=${TOOLCHAIN_ROOT}/usr/bin/ld
    export LDFLAGS="-isysroot ${SYSROOT} ${DEV_ARCHS} -lstdc++"
    
  else
    export CC=${TOOLCHAIN_ROOT}/usr/bin/gcc
    export CXX=${TOOLCHAIN_ROOT}/usr/bin/g++
    export LD=${TOOLCHAIN_ROOT}/usr/bin/ld\ -r
    export CPP=${TOOLCHAIN_ROOT}/usr/bin/cpp
    export CXXCPP=${TOOLCHAIN_ROOT}/usr/bin/cpp
    export LDFLAGS="-isysroot ${SYSROOT} ${DEV_ARCHS}"
    export AR=${TOOLCHAIN_ROOT}/usr/bin/ar
    export AS=${TOOLCHAIN_ROOT}/usr/bin/as
    export LIBTOOL=${TOOLCHAIN_ROOT}/usr/bin/libtool
    export STRIP=${TOOLCHAIN_ROOT}/usr/bin/strip
    export RANLIB=${TOOLCHAIN_ROOT}/usr/bin/ranlib
  fi

  if [ ! -d ${SYSROOT} ]; then
    echo
    echo "Cannot find iOS developer tools at ${SYSROOT}."
    echo
    exit  
  fi

  if [ -f Makefile ]; then
    make clean
  fi

  mkdir -p ${DEST_DIR}/device &> /dev/null

  ./configure \
  CFLAGS="-O -isysroot ${SYSROOT} ${DEV_ARCHS}" \
  CXXFLAGS="-O -isysroot ${SYSROOT} ${DEV_ARCHS}" \
  --disable-dependency-tracking \
  --host=arm-apple-darwin10 \
  --target=arm-apple-darwin10 \
  --disable-shared \
  --enable-utf8 \
  --prefix=${DEST_DIR}/device

  make -j2 install
else
  echo
  echo "${DEST_DIR}/device already exists - not rebuilding."
  echo
fi

#
# Simulator
#
if [ ! -d ${DEST_DIR}/simulator ]; then

  TOOLCHAIN_ROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer" 
  SYSROOT="${TOOLCHAIN_ROOT}/SDKs/iPhoneSimulator${SDK_VER}.sdk"

  if [ $(bc <<< "$SDK_VER >= 7.0") -eq 1 ]; then
    # export CXX=${TOOLCHAIN_ROOT}/usr/bin/g++
    # export CC=${TOOLCHAIN_ROOT}/usr/bin/gcc
    # export AR=/Applications/Xcode.app/Contents/Developer/usr/bin/ar
    # export AS=/Applications/Xcode.app/Contents/Developer/usr/bin/as
    # export LIBTOOL=/Applications/Xcode.app/Contents/Developer/usr/bin/libtool
    # export STRIP=/Applications/Xcode.app/Contents/Developer/usr/bin/strip
    # export RANLIB=/Applications/Xcode.app/Contents/Developer/usr/bin/ranlib
    # export LD=${TOOLCHAIN_ROOT}/usr/bin/ld
    export LDFLAGS="-isysroot ${SYSROOT} -arch i386 -lstdc++"
  else
    export CXX=${TOOLCHAIN_ROOT}/usr/bin/llvm-g++
    export CC=${TOOLCHAIN_ROOT}/usr/bin/llvm-gcc
    export AR=${TOOLCHAIN_ROOT}/usr/bin/ar
    export AS=${TOOLCHAIN_ROOT}/usr/bin/as
    export LIBTOOL=${TOOLCHAIN_ROOT}/usr/bin/libtool
    export LDFLAGS="-isysroot  ${SYSROOT} -arch i386"
    export STRIP=${TOOLCHAIN_ROOT}/usr/bin/strip
    export RANLIB=${TOOLCHAIN_ROOT}/usr/bin/ranlib
    export LD=${TOOLCHAIN_ROOT}/usr/bin/ld\ -r
  fi

  if [ ! -d ${SYSROOT} ]; then
    echo
    echo "Cannot find iOS developer tools at ${SYSROOT}."
    echo
    exit  
  fi

  if [ -f Makefile ]; then
  	make clean
  fi

  mkdir -p ${DEST_DIR}/simulator &> /dev/null

  ./configure \
  CFLAGS="-O -isysroot ${SYSROOT} -arch i386" \
  CXXFLAGS="-O -isysroot ${SYSROOT} -arch i386" \
  --disable-dependency-tracking \
  --disable-shared \
  --enable-utf8 \
  --prefix=${DEST_DIR}/simulator

  make -j2 install
else
  echo
  echo "${DEST_DIR}/device already exists - not rebuilding."
  echo
fi

cp ${DEST_DIR}/device/include/* ${DIR}/../prebuilt/ios/include

echo
echo "- Creating universal binaries --------------------------------------"
echo

LIBS=`find ${DIR}/../prebuilt/ios/*pcre-build* -name *.a`
set +e
for LIB in ${LIBS}; do
  LIB_BASE=`basename $LIB .a`
  ARCHS=`xcrun -sdk iphoneos lipo -info $LIB`
  ARCHS=`expr "$ARCHS" : '.*:\(.*\)$'`
  for ARCH in ${ARCHS}; do
    mkdir -p ${DIR}/../prebuilt/ios/arch/${ARCH} > /dev/null
    xcrun -sdk iphoneos lipo -extract $ARCH $LIB -output ${DIR}/../prebuilt/ios/arch/${ARCH}/${LIB_BASE}.a \
      || cp $LIB ${DIR}/../prebuilt/ios/arch/${ARCH}/${LIB_BASE}.a
    UNIQUE_LIBS=`ls ${DIR}/../prebuilt/ios/arch/${ARCH}`
  done
done


for LIB in ${UNIQUE_LIBS}; do
  FILELIST=""
  for ARCH in `ls ${DIR}/../prebuilt/ios/arch/`; do
    FILELIST="${FILELIST} ${DIR}/../prebuilt/ios/arch/${ARCH}/${LIB}"
  done
  xcrun -sdk iphoneos lipo -create ${FILELIST} -output ${DIR}/../prebuilt/ios/lib/${LIB}
done

rm -rf ${DIR}/../prebuilt/ios/arch/
