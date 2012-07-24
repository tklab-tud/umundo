#!/bin/bash

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"

# do not tar ._ files
export COPY_EXTENDED_ATTRIBUTES_DISABLE=1
export COPYFILE_DISABLE=1

############################
# Compile libraries
############################

echo == BUILDING UMUNDO FOR MacOSX =========================================================
rm -rf /tmp/build-umundo
mkdir -p /tmp/build-umundo
cd /tmp/build-umundo
cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Debug ${DIR}/../..
make -j2
cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release ${DIR}/../..
make -j2

cd ${DIR}

echo Start the Linux 32Bit system named 'debian' && read
echo == BUILDING UMUNDO FOR Linux 32Bit =========================================================
export UMUNDO_BUILD_HOST=debian
expect build-linux.expect

echo Start the Linux 64Bit system named 'debian64' && read
echo == BUILDING UMUNDO FOR Linux 64Bit =========================================================
export UMUNDO_BUILD_HOST=debian64
expect build-linux.expect

echo Start the Windows 32Bit system named 'epikur-win7' && read
echo == BUILDING UMUNDO FOR Windows 32Bit =========================================================
export UMUNDO_BUILD_HOST=epikur-win7
# winsshd needs an xterm ..
TERM=xterm expect build-windows.expect

echo == BUILDING UMUNDO FOR IOS =========================================================
${DIR}/../build-umundo-ios.sh

echo == BUILDING UMUNDO FOR Android =========================================================
${DIR}/../build-umundo-android.sh

############################
# Create installers
############################

echo == PACKAGING UMUNDO FOR MacOSX =========================================================
cd /tmp/build-umundo
# rerun cmake for new cpack files
cmake -DDIST_PREPARE=ON -DCMAKE_BUILD_TYPE=Release ${DIR}/../..
make package
cp umundo*darwin* ${DIR}/../../installer
cd ${DIR}

echo Start the Linux 32Bit system named 'debian' again && read
echo == PACKAGING UMUNDO FOR Linux 32Bit =========================================================
export UMUNDO_BUILD_HOST=debian
expect package-linux.expect

echo Start the Linux 64Bit system named 'debian64' again && read
echo == PACKAGING UMUNDO FOR Linux 64Bit =========================================================
export UMUNDO_BUILD_HOST=debian64
expect package-linux.expect
 
echo Start the Windows 32Bit system named 'epikur-win7' again && read
echo == PACKAGING UMUNDO FOR Windows 32Bit =========================================================
export UMUNDO_BUILD_HOST=epikur-win7
TERM=xterm expect package-windows.expect

############################
# Validate installers
############################

expect validate-installers.expect