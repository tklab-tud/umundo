#!/bin/bash

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
PREBUILT="$( cd "$( dirname "$1" )" && pwd )"
CWD=`pwd`

cd $DIR

if [ "$UMUNDO_PREBUILT_HOST" == "" ]; then
	UMUNDO_PREBUILT_HOST="admin@umundo.tk.informatik.tu-darmstadt.de:/var/www/html/umundo/prebuilt"
fi

if [ "$1" == "" ] || [ "$2" == "" ]; then
	echo "$ME <prebuilt dir> <version>"
	exit
fi

if [ ! -d $PREBUILT ]; then
	echo "$PREBUILT: no such directory"
	exit
fi

VERSION=$2

cd ../prebuilt

PLATFORMS=`find . -maxdepth 1 -type d -regex ./[^\.].*`
for FILE in ${PLATFORMS}; do
  PLATFORM=`basename $FILE`
    echo $PLATFORM
    tar cvzf umundo-prebuilt-${PLATFORM}.tgz ${FILE}
    scp umundo-prebuilt-${PLATFORM}.tgz ${UMUNDO_PREBUILT_HOST}/${VERSION}
#    rm umundo-prebuilt-${PLATFORM}.tgz
done