#!/bin/bash

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"

# do not tar ._ files
export COPY_EXTENDED_ATTRIBUTES_DISABLE=1
export COPYFILE_DISABLE=1

############################
# Compile libraries
############################

cd ${DIR}

echo -n "Build umundo for Linux 32Bit? [y/N]: "; read BUILD_LINUX32
if [ "$BUILD_LINUX32" == "y" ] || [ "$BUILD_LINUX32" == "Y" ]; then
	echo "Start the Linux 32Bit system named 'debian' and press return" && read
	echo == BUILDING UMUNDO FOR Linux 32Bit =========================================================
	export UMUNDO_BUILD_HOST=debian
	expect build-linux.expect
fi

echo -n "Build umundo for Linux 64Bit? [y/N]: "; read BUILD_LINUX64
if [ "$BUILD_LINUX64" == "y" ] || [ "$BUILD_LINUX64" == "Y" ]; then
	echo "Start the Linux 64Bit system named 'debian64' and press return" && read
	echo == BUILDING UMUNDO FOR Linux 64Bit =========================================================
	export UMUNDO_BUILD_HOST=debian64
	expect build-linux.expect
fi

# make sure to cross-compile before windows as we will copy all the files into the windows VM
echo -n "Build umundo for iOS? [y/N]: "; read BUILD_IOS
if [ "$BUILD_IOS" == "y" ] || [ "$BUILD_IOS" == "Y" ]; then
	echo == BUILDING UMUNDO FOR IOS =========================================================
	${DIR}/../build-umundo-ios.sh
fi

echo -n "Build umundo for Android? [y/N]: "; read BUILD_ANDROID
if [ "$BUILD_ANDROID" == "y" ] || [ "$BUILD_ANDROID" == "Y" ]; then
	echo == BUILDING UMUNDO FOR Android =========================================================
	export ANDROID_NDK=/Developer/Applications/android-ndk-r8
	${DIR}/../build-umundo-android.sh
fi

echo -n "Build umundo for Windows 32Bit? [y/N]: "; read BUILD_WIN32
if [ "$BUILD_WIN32" == "y" ] || [ "$BUILD_WIN32" == "Y" ]; then
	echo "Start the Windows 32Bit system named 'epikur-win7' and press return" && read
	echo == BUILDING UMUNDO FOR Windows 32Bit =========================================================
	export UMUNDO_BUILD_HOST=epikur-win7
	export UMUNDO_BUILD_ARCH=32
	# winsshd needs an xterm ..
	TERM=xterm expect build-windows.expect
fi

echo -n "Build umundo for Windows 64Bit? [y/N]: "; read BUILD_WIN64
if [ "$BUILD_WIN64" == "y" ] || [ "$BUILD_WIN64" == "Y" ]; then
	echo "Start the Windows 64Bit system named 'epikur-win7-64' and press return" && read
	echo == BUILDING UMUNDO FOR Windows 64Bit =========================================================
	export UMUNDO_BUILD_HOST=epikur-win7-64
	export UMUNDO_BUILD_ARCH=64
	# winsshd needs an xterm ..
	TERM=xterm expect build-windows.expect
fi

echo -n "Build umundo for Mac OSX? [y/N]: "; read BUILD_MAC
if [ "$BUILD_MAC" == "y" ] || [ "$BUILD_MAC" == "Y" ]; then
	echo == BUILDING UMUNDO FOR Mac OSX =========================================================
	rm -rf /tmp/build-umundo
	mkdir -p /tmp/build-umundo
	cd /tmp/build-umundo
	cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Debug ${DIR}/../..
	make -j2
	make -j2 java	
	cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release ${DIR}/../..
	make -j2
	make -j2 java	
fi

############################
# Create installers
############################

echo -n "Build packages for those platforms? [a/y/N]: "; read BUILD_PACKAGES
if [ "$BUILD_PACKAGES" == "y" ] || [ "$BUILD_PACKAGES" == "a" ]; then

	if [ "$BUILD_LINUX32" == "y" ] || [ "$BUILD_LINUX32" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
		echo Start the Linux 32Bit system named 'debian' again && read
		echo == PACKAGING UMUNDO FOR Linux 32Bit =========================================================
		export UMUNDO_BUILD_HOST=debian
		expect package-linux.expect
	fi

	if [ "$BUILD_LINUX64" == "y" ] || [ "$BUILD_LINUX64" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
		echo Start the Linux 64Bit system named 'debian64' again && read
		echo == PACKAGING UMUNDO FOR Linux 64Bit =========================================================
		export UMUNDO_BUILD_HOST=debian64
		expect package-linux.expect
 fi

	if [ "$BUILD_WIN32" == "y" ] || [ "$BUILD_WIN32" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
		echo Start the Windows 32Bit system named 'epikur-win7' again && read
		echo == PACKAGING UMUNDO FOR Windows 32Bit =========================================================
		export UMUNDO_BUILD_HOST=epikur-win7
		export UMUNDO_BUILD_ARCH=32
		TERM=xterm expect package-windows.expect
	fi
	
	if [ "$BUILD_WIN64" == "y" ] || [ "$BUILD_WIN64" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
		echo Start the Windows 64Bit system named 'epikur-win7-64' again && read
		echo == PACKAGING UMUNDO FOR Windows 64Bit =========================================================
		export UMUNDO_BUILD_HOST=epikur-win7-64
		export UMUNDO_BUILD_ARCH=64
		TERM=xterm expect package-windows.expect
	fi

	if [ "$BUILD_MAC" == "y" ] || [ "$BUILD_MAC" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
		echo == PACKAGING UMUNDO FOR MacOSX =========================================================
		cd /tmp/build-umundo
		# rerun cmake for new cpack files
		cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release ${DIR}/../..
		make package
		cp umundo*darwin* ${DIR}/../../installer
		cd ${DIR}
	fi

	############################
	# Validate installers
	############################

	expect validate-installers.expect

fi