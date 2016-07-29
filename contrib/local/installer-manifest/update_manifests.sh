#!/bin/bash

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )" 
SRC_DIR="${DIR}/../../../installer"

# windows x86
unzip -q ${SRC_DIR}/umundo-windows-x86-*.zip
cd umundo-windows-x86-*
find . -exec file -h {} \; |grep -v "\.:" > ${DIR}/windows-x86.txt
cd ..
rm -rf ${DIR}/../umundo-windows-x86-*

# windows x86_64
unzip -q ${SRC_DIR}/umundo-windows-x86_64-*.zip
cd umundo-windows-x86_64-*
find . -exec file -h {} \; |grep -v "\.:" > ${DIR}/windows-x86_64.txt
cd ..
rm -rf ${DIR}/../umundo-windows-x86_64-*

# darwin i386
tar xzf ${SRC_DIR}/umundo-darwin-i386-*.tar.gz
cd umundo-darwin-i386-*
find . -exec file -h {} \; |grep -v "\.:" > ${DIR}/darwin-i386.txt
cd ..
rm -rf ${DIR}/../umundo-darwin-i386-*

# linux armv6l
tar xzf ${SRC_DIR}/umundo-linux-armv6l-*.tar.gz
cd umundo-linux-armv6l-*
find . -exec file -h {} \; |grep -v "\.:" > ${DIR}/linux-armv6l.txt
cd ..
rm -rf ${DIR}/../umundo-linux-armv6l-*

# linux i686
tar xzf ${SRC_DIR}/umundo-linux-i686-*.tar.gz
cd umundo-linux-i686-*
find . -exec file -h {} \; |grep -v "\.:" > ${DIR}/linux-i686.txt
cd ..
rm -rf ${DIR}/../umundo-linux-i686-*

# linux x86_64
tar xzf ${SRC_DIR}/umundo-linux-x86_64-*.tar.gz
cd umundo-linux-x86_64-*
find . -exec file -h {} \; |grep -v "\.:" > ${DIR}/linux-x86_64.txt
cd ..
rm -rf ${DIR}/../umundo-linux-x86_64-*

exit

