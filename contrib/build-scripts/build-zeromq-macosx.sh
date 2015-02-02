#!/bin/bash

#
# build ZeroMQ for iOS and iOS simulator
#

# exit on error
set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 
MACOSX_VER=`/usr/bin/sw_vers -productVersion`
MACOSX_COMP=(`echo $MACOSX_VER | tr '.' ' '`)
BUILD_DIR="/tmp/zeromq"

if [ ! -f src/zmq.cpp ]; then
	echo
	echo "Cannot find src/zmq.cpp"
	echo "Run script from within zeroMQ directory:"
	echo "zeromq-4.1.0$ ../../${ME}"
	echo
	exit
fi

if [ ${MACOSX_COMP[1]} -lt 9 ]; then
  MACOSX_VERSION_MIN="-mmacosx-version-min=10.6"
else
  MACOSX_VERSION_MIN="-mmacosx-version-min=10.7"
fi

if [ -f Makefile ]; then
	make clean
fi


declare -a cpus=("i386" "x86_64")
declare -a libcpps=("libstdc++" "libc++")
declare -a debugs=("_d" "")

# now loop through the above array
for cpu in "${cpus[@]}"; do
	for libcpp in "${libcpps[@]}"; do
		for debug in "${debugs[@]}"; do
			PLATFORM=darwin-${cpu}-clang-${libcpp}

			mkdir -p ${BUILD_DIR}/${PLATFORM}${debug} &> /dev/null

			case "$debug" in
				_d )
					./configure \
					CFLAGS="-g ${MACOSX_VERSION_MIN} -arch ${cpu} -stdlib=${libcpp}" \
					CXXFLAGS="-g ${MACOSX_VERSION_MIN} -arch ${cpu} -stdlib=${libcpp}" \
					LDFLAGS="${MACOSX_VERSION_MIN} -arch ${cpu} -stdlib=${libcpp}" \
					--enable-static \
					--without-documentation \
					--prefix=${BUILD_DIR}/${PLATFORM}_d
				;;
				* )
					./configure \
					CFLAGS="-Os ${MACOSX_VERSION_MIN} -arch ${cpu} -stdlib=${libcpp}" \
					CXXFLAGS="-Os ${MACOSX_VERSION_MIN} -arch ${cpu} -stdlib=${libcpp}" \
					LDFLAGS="${MACOSX_VERSION_MIN} -arch ${cpu} -stdlib=${libcpp}" \
					--enable-static \
					--without-documentation \
					--prefix=${BUILD_DIR}/${PLATFORM}
				;;
			esac
			
			# link-time failures otherwise
			sed -i.orig -e 's/define HAVE_LIBGSSAPI_KRB5 1/undef HAVE_LIBGSSAPI_KRB5/' src/platform.hpp

			make -j2 install
			make clean

		done
	done
done

# copy headers
for libcpp in "${libcpps[@]}"; do
	for cpu in "${cpus[@]}"; do
		TARGET_CPU=${cpu//i386/x86}
		echo $TARGET_CPU

		cp -R ${BUILD_DIR}/darwin-${cpu}-clang-${libcpp}/include/* \
			${DIR}/../prebuilt/darwin-${TARGET_CPU}-clang-${libcpp}/include
		cp -R ${BUILD_DIR}/darwin-${cpu}-clang-${libcpp}/include/* \
			${DIR}/../prebuilt/darwin-${TARGET_CPU}-gnu-${libcpp}/include
	done
done

# create universal binaries
for libcpp in "${libcpps[@]}"; do
	for debug in "${debugs[@]}"; do
		LIPO_INPUT=""
		for cpu in "${cpus[@]}"; do
			PLATFORM=darwin-${cpu}-clang-${libcpp}
			LIPO_INPUT="${LIPO_INPUT} ${BUILD_DIR}/${PLATFORM}${debug}/lib/libzmq.a"
		done
		LIPO_OUTPUT=${DIR}/../prebuilt/darwin-x86_64-clang-${libcpp}/lib/libzmq${debug}.a
		lipo -create ${LIPO_INPUT} -output ${LIPO_OUTPUT}
		lipo -info ${LIPO_OUTPUT}
		cp ${LIPO_OUTPUT} ${DIR}/../prebuilt/darwin-x86-clang-${libcpp}/lib/libzmq${debug}.a
		cp ${LIPO_OUTPUT} ${DIR}/../prebuilt/darwin-x86_64-gnu-${libcpp}/lib/libzmq${debug}.a
		cp ${LIPO_OUTPUT} ${DIR}/../prebuilt/darwin-x86-gnu-${libcpp}/lib/libzmq${debug}.a
	done
done
