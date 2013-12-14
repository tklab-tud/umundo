#!/bin/bash

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

cd $DIR

if [ "$UMUNDO_PREBUILT_HOST" == "" ]; then
	UMUNDO_PREBUILT_HOST="admin@umundo.tk.informatik.tu-darmstadt.de"
fi

UMUNDO_PREBUILT_PATH="/var/www/html/umundo/prebuilt"

if [ "$1" == "" ] || [ "$2" == "" ]; then
	echo "$ME <prebuilt dir> <version>"
	exit
fi

if [ ! -d $1 ]; then
	echo "$1: no such directory"
	exit
fi

VERSION=$2

cd ../prebuilt

ssh ${UMUNDO_PREBUILT_HOST} mkdir -p ${UMUNDO_PREBUILT_PATH}/${VERSION}

PLATFORMS=`find . -maxdepth 1 -type d -regex ./[^\.].*`
#PLATFORMS="linux-x86_64"
for FILE in ${PLATFORMS}; do
  PLATFORM=`basename $FILE`
  echo $FILE
  if [ "$PLATFORM" != "include" ]; then
    if [[ "$PLATFORM" == windows* ]]; then
      tar cvzf umundo-prebuilt-${PLATFORM}.tgz --exclude='*/.DS_Store' --exclude='VERSION.txt' ${FILE}
    else
      tar cvzf umundo-prebuilt-${PLATFORM}.tgz --exclude='*/.DS_Store' --exclude='VERSION.txt' --exclude='lib/*_d.a' ${FILE}
    fi
    scp umundo-prebuilt-${PLATFORM}.tgz ${UMUNDO_PREBUILT_HOST}:${UMUNDO_PREBUILT_PATH}/${VERSION}
    rm umundo-prebuilt-${PLATFORM}.tgz
  else
    tar cvzf umundo-prebuilt-include.tgz --exclude='*/.DS_Store' --exclude='VERSION.txt' ${FILE}
    scp umundo-prebuilt-include.tgz ${UMUNDO_PREBUILT_HOST}:${UMUNDO_PREBUILT_PATH}/${VERSION}
    rm umundo-prebuilt-include.tgz
  fi
done