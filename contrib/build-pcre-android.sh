#!/bin/bash

#
# build PCRE for android
#

# exit on error
# exit on error
set -e

ME=`basename $0`
SDK_VER="5.0"
TARGET_DEVICE="arm-linux-androideabi"
DEST_DIR="${PWD}/../../prebuilt/android/${TARGET_DEVICE}/lib"
BUILD_DIR="/tmp/build-pcre-android/"


if [ ! -f pcre_version.c ]; then
	echo
	echo "Cannot find pcre_version.c"
	echo "Run script from within pcre directory:"
	echo "pcre-8.31$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

# set ANDROID_NDK to default if environment variable not set
if [ ! -f ${ANDROID_NDK_ROOT}/ndk-build ]; then
	# try some convinient default locations
	if [ -d /opt/android-ndk-r7 ]; then
		ANDROID_NDK_ROOT="/opt/android-ndk-r7"
	elif [ -d /Developer/Applications/android-ndk-r7 ]; then
		ANDROID_NDK_ROOT="/Developer/Applications/android-ndk-r7"
	elif [ -d /home/sradomski/Documents/android-dev/android-ndk-r7 ]; then
		ANDROID_NDK_ROOT="/home/sradomski/Documents/android-dev/android-ndk-r7"
	else
		echo
		echo "Cannot find android-ndk, call script as"
		echo "ANDROID_NDK_ROOT=\"/path/to/android/ndk\" ${ME}"
		echo
		exit
	fi
	export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}"
fi

# download updated config.guess and config.sub
#cd config
#wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O config.guess
#wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O config.sub
#cd ..

if [ -f Makefile ]; then
	make clean
fi


# if [ -f ispatched ]; then
# 	rm ./ispatched
# else
# 	patch -p1 < ../zeromq-3.1.0.android.patch
# fi
# touch ispatched


# select and verify toolchain
ANDROID_SYS_ROOT="${ANDROID_NDK_ROOT}/standalone"

if [ ! -f ${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-gcc ]; then
	echo
	echo "Cannot find compiler at ${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-gcc"
	echo
	exit;
fi

if [ ! -d ${ANDROID_SYS_ROOT}/lib ]; then
	echo
	echo "Cannot find sytem libraries at ${ANDROID_SYS_ROOT}/lib"
	echo
	exit;
fi

COMMON_FLAGS=" -DANDROID -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -DTARGET_OS_ANDROID -fsigned-char -mfloat-abi=softfp -mfpu=vfp -fno-exceptions -fno-rtti -mthumb -fomit-frame-pointer -fno-strict-aliasing "

./configure \
CPP="cpp" \
CXXCPP="cpp" \
CC="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-gcc" \
CXX="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-g++" \
LD="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-ld" \
CXXFLAGS="-Wno-sign-compare -Os -s ${COMMON_FLAGS}" \
CFLAGS="-Wno-sign-compare -Os -s ${COMMON_FLAGS}" \
LIBZMQ_EXTRA_LDFLAGS="-llog" \
LDFLAGS="-L${ANDROID_SYS_ROOT}/lib -static -Wl,--fix-cortex-a8" \
CPPFLAGS="-I${ANDROID_SYS_ROOT}/include -static" \
AR="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-ar" \
AS="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-as" \
LIBTOOL="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-libtool" \
STRIP="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-strip" \
RANLIB="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-ranlib" \
--disable-dependency-tracking \
--target=arm-linux-androideabi \
--host=arm-linux-androideabi \
--enable-static \
--disable-shared \
--prefix=${BUILD_DIR}

make -j2 install

mv ${BUILD_DIR}/lib/libpcre.a ${DEST_DIR}/libpcre.a
mv ${BUILD_DIR}/lib/libpcreposix.a ${DEST_DIR}/libpcreposix.a

make clean

# build again with debugging
./configure \
CPP="cpp" \
CXXCPP="cpp" \
CC="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-gcc" \
CXX="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-g++" \
LD="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-ld" \
CXXFLAGS="-Wno-sign-compare -g ${COMMON_FLAGS}" \
CFLAGS="-Wno-sign-compare -g ${COMMON_FLAGS}" \
LIBZMQ_EXTRA_LDFLAGS="-llog" \
LDFLAGS="-L${ANDROID_SYS_ROOT}/lib -static -Wl,--fix-cortex-a8" \
CPPFLAGS="-I${ANDROID_SYS_ROOT}/include -static" \
AR="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-ar" \
AS="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-as" \
LIBTOOL="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-libtool" \
STRIP="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-strip" \
RANLIB="${ANDROID_SYS_ROOT}/bin/${TARGET_DEVICE}-ranlib" \
--disable-dependency-tracking \
--target=arm-linux-androideabi \
--host=arm-linux-androideabi \
--enable-static \
--disable-shared \
--prefix=${BUILD_DIR}

make -j2 install

mv ${BUILD_DIR}/lib/libpcre.a ${DEST_DIR}/libpcre_d.a
mv ${BUILD_DIR}/lib/libpcreposix.a ${DEST_DIR}/libpcreposix_d.a

echo
echo "Installed static libraries in ${DEST_DIR}"
echo