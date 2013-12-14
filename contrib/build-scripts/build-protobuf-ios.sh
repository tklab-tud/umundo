#!/bin/bash

#
# build ProtoBuf for iOS and iOS simulator
#

PROTOC=`which protoc`
# make sure this is not set
unset MACOSX_DEPLOYMENT_TARGET

# be ridiculously conservative with regard to ios features
export IPHONEOS_DEPLOYMENT_TARGET="4.3"

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
#SDK_VER="6.1"
SDK_VER="7.0"
DEST_DIR="${DIR}/../prebuilt/ios/${SDK_VER}-protobuf-build"

if [ ! -d src/google/protobuf ]; then
	echo
	echo "Cannot find src/google/protobuf"
	echo "Run script from within protobuf directory:"
	echo "protobuf-2.4.1$ ../../../${ME}"
	echo
	exit
fi

mkdir -p ${DEST_DIR} &> /dev/null

if [ ! -n "${PROTOC}" ]; then
	echo
	echo "Cannot find native protoc compiler"
	echo "make sure 'which protoc' returns a path"
  echo
	exit
fi

# see http://stackoverflow.com/questions/2424770/floating-point-comparison-in-shell-script
if [ $(bc <<< "$SDK_VER >= 7.0") -eq 1 ]; then
  DEV_ARCHS="-arch armv7 -arch armv7s" # cant build 64bit in one swoop, see below
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
  --enable-shared=no \
  --with-protoc=${PROTOC} \
  --prefix=${DEST_DIR}/device

  make -j2 install

  cp -R ${DEST_DIR}/device/include/* ${DIR}/../prebuilt/ios/include

  if [ $(bc <<< "$SDK_VER >= 7.0") -eq 1 ]; then
  	make distclean
    
    curl https://gist.github.com/BennettSmith/7111094/raw/171695f70b102de2301f5b45d9e9ab3167b4a0e8/0001-Add-generic-GCC-support-for-atomic-operations.patch --output /tmp/0001-Add-generic-GCC-support-for-atomic-operations.patch
    curl https://gist.github.com/BennettSmith/7111094/raw/a4e85ffc82af00ae7984020300db51a62110db48/0001-Add-generic-gcc-header-to-Makefile.am.patch --output /tmp/0001-Add-generic-gcc-header-to-Makefile.am.patch
    patch -p1 < /tmp/0001-Add-generic-GCC-support-for-atomic-operations.patch
    patch -p1 < /tmp/0001-Add-generic-gcc-header-to-Makefile.am.patch
    
    export LDFLAGS="-isysroot ${SYSROOT} -arch arm64 -lstdc++"
    
    mkdir -p ${DEST_DIR}/device64 &> /dev/null
    
    ./configure \
    CFLAGS="-O -isysroot ${SYSROOT} -arch arm64" \
    CXXFLAGS="-O -isysroot ${SYSROOT} -arch arm64" \
    --disable-dependency-tracking \
    --host=arm-apple-darwin10 \
    --target=arm-apple-darwin10 \
    --enable-shared=no \
    --with-protoc=${PROTOC} \
    --prefix=${DEST_DIR}/device64

    make -j2 install
    
    patch -Rp1 < /tmp/0001-Add-generic-GCC-support-for-atomic-operations.patch
    patch -Rp1 < /tmp/0001-Add-generic-gcc-header-to-Makefile.am.patch
    
  fi
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
  --enable-shared=no \
  --with-protoc=${PROTOC} \
  --prefix=${DEST_DIR}/simulator

  make -j2 install
else
  echo
  echo "${DEST_DIR}/device already exists - not rebuilding."
  echo
fi

echo
echo "- Creating universal binaries --------------------------------------"
echo

LIBS=`find ${DIR}/../prebuilt/ios/*protobuf-build* -name *.a`
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
