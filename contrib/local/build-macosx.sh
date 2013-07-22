#!/bin/bash

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"

mkdir -p /tmp/build-umundo && cd /tmp/build-umundo

###############################
# Language Bindings and convenience libraries
#
rm -rf /tmp/build-umundo/*
cmake \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 \
  -DBUILD_UMUNDO_APPS=OFF \
  -DBUILD_UMUNDO_TOOLS=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_BINDINGS=ON \
  -DDIST_PREPARE=ON \
  -DBUILD_PREFER_STATIC_LIBRARIES=ON \
  -DBUILD_CONVENIENCE_LIB=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  ${DIR}/../..
make -j2

rm -rf /tmp/build-umundo/*
cmake \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 \
  -DBUILD_UMUNDO_APPS=OFF \
  -DBUILD_UMUNDO_TOOLS=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_BINDINGS=ON \
  -DDIST_PREPARE=ON \
  -DBUILD_PREFER_STATIC_LIBRARIES=ON \
  -DBUILD_CONVENIENCE_LIB=ON \
  -DCMAKE_BUILD_TYPE=Release \
  ${DIR}/../..
make -j2
make -j2 java

###############################
# Static libraries
#
rm -rf /tmp/build-umundo/*
cmake \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 \
  -DBUILD_UMUNDO_APPS=OFF \
  -DBUILD_UMUNDO_TOOLS=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_BINDINGS=OFF \
  -DDIST_PREPARE=ON \
  -DBUILD_PREFER_STATIC_LIBRARIES=OFF \
  -DCMAKE_BUILD_TYPE=Debug \
  ${DIR}/../..
make -j2

rm -rf /tmp/build-umundo/*
cmake \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 \
  -DBUILD_UMUNDO_APPS=OFF \
  -DBUILD_UMUNDO_TOOLS=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_BINDINGS=OFF \
  -DDIST_PREPARE=ON \
  -DBUILD_PREFER_STATIC_LIBRARIES=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  ${DIR}/../..
make -j2

###############################
# Shared libraries
#
rm -rf /tmp/build-umundo/*
cmake \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 \
  -DBUILD_UMUNDO_APPS=OFF \
  -DBUILD_UMUNDO_TOOLS=OFF \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_BINDINGS=OFF \
  -DDIST_PREPARE=ON \
  -DBUILD_CONVENIENCE_LIB=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  ${DIR}/../..
make -j2

rm -rf /tmp/build-umundo/*
cmake \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 \
  -DBUILD_UMUNDO_APPS=OFF \
  -DBUILD_UMUNDO_TOOLS=ON \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_BINDINGS=OFF \
  -DDIST_PREPARE=ON \
  -DBUILD_CONVENIENCE_LIB=ON \
  -DCMAKE_BUILD_TYPE=Release \
  ${DIR}/../..
make -j2
