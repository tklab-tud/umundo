#!/bin/bash

#
# build Avahi without all the pesky dependencies
#

echo "Deprecated - we expect system-wide libraries for avahi clients"
exit

# exit on error
set -e

ME=`basename $0`
DEST_DIR="${PWD}/../../prebuilt/avahi/linux-i686/gnu"

if [ ! -d avahi-core ]; then
	echo
	echo "Cannot find avahi-core/"
	echo "Run script from within avahi directory:"
	echo "avahi-0.6.30$ ../../${ME}"
	echo
	exit
fi
mkdir -p ${DEST_DIR} &> /dev/null

chmod 755 ./configure

if [ -f Makefile ]; then
	make clean
fi

./configure \
--disable-manpages \
--disable-xmltoman \
--disable-glib \
--disable-gobject \
--disable-qt3 \
--disable-qt4 \
--disable-gtk \
--disable-gtk3 \
--disable-dbus \
--disable-gdbm \
--disable-libdaemon \
--disable-python \
--disable-pygtk \
--disable-python-dbus \
--disable-mono \
--disable-monodoc \
--disable-autoipd \
--disable-doxygen-doc \
--disable-doxygen-dot \
--disable-doxygen-xml \
--disable-doxygen-html \
--enable-static \
--enable-shared \
--prefix=${DEST_DIR} \

exit

make -j2 install

# tidy up
rm -rf ${DEST_DIR}/bin
rm -rf ${DEST_DIR}/etc
rm -rf ${DEST_DIR}/lib/avahi
rm -rf ${DEST_DIR}/lib/pkgconfig
rm -rf ${DEST_DIR}/sbin
rm -rf ${DEST_DIR}/share
rm -rf ${DEST_DIR}/var
mkdir -p ${DEST_DIR}/../../../include/avahi
mv ${DEST_DIR}/include/* ${DEST_DIR}/../../../include/avahi
mv ${DEST_DIR}/lib/* ${DEST_DIR}
rm -rf ${DEST_DIR}/include