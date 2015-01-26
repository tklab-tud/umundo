#!/bin/bash

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

cd $DIR

if [ "$UMUNDO_PREBUILT_HOST" == "" ]; then
	UMUNDO_PREBUILT_HOST="sradomski@umundo.tk.informatik.tu-darmstadt.de"
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
#PLATFORMS="darwin-i386"
for FILE in ${PLATFORMS}; do
	PLATFORM=`basename $FILE`
	echo $FILE
	case "$PLATFORM" in
	*linux-*-clang* | *darwin-*-gnu* )
	# do nothing - we will symlink
	;;
	"include")
		tar cvzf include.tgz --exclude='*/.DS_Store' --exclude='VERSION.txt' ${FILE}
		scp include.tgz ${UMUNDO_PREBUILT_HOST}:${UMUNDO_PREBUILT_PATH}/${VERSION}
		rm include.tgz
		;;
	*darwin*)
		cd $FILE
		# do not upload v8 for mac and strip first dir
		tar cvzf ../umundo-prebuilt-${PLATFORM}.tgz --exclude='*/.DS_Store' --exclude='VERSION.txt' *
		cd ..
		scp umundo-prebuilt-${PLATFORM}.tgz ${UMUNDO_PREBUILT_HOST}:${UMUNDO_PREBUILT_PATH}/${VERSION}
		rm umundo-prebuilt-${PLATFORM}.tgz
		;;
	*linux*)
		cd $FILE
		# no debug libs with linux and strip first dir
		tar cvzf ../umundo-prebuilt-${PLATFORM}.tgz --exclude='*/.DS_Store' --exclude='VERSION.txt' *
		cd ..
		scp umundo-prebuilt-${PLATFORM}.tgz ${UMUNDO_PREBUILT_HOST}:${UMUNDO_PREBUILT_PATH}/${VERSION}
		rm umundo-prebuilt-${PLATFORM}.tgz
		;;
	*)
		cd $FILE
		# and strip first dir
		tar cvzf ../umundo-prebuilt-${PLATFORM}.tgz --exclude='*/.DS_Store' --exclude='VERSION.txt' *
		cd ..
		scp umundo-prebuilt-${PLATFORM}.tgz ${UMUNDO_PREBUILT_HOST}:${UMUNDO_PREBUILT_PATH}/${VERSION}
		rm umundo-prebuilt-${PLATFORM}.tgz
		;;
	esac
done

# link ABI compatibles

for FILE in ${PLATFORMS}; do
	PLATFORM=`basename $FILE`

	case "$PLATFORM" in
		*linux-*-gnu* )
			# gcc is ABI compatible to clang
			NEW_PLATFORM="${PLATFORM//gnu/clang}"
			ssh ${UMUNDO_PREBUILT_HOST} \
				ln -s ${UMUNDO_PREBUILT_PATH}/${VERSION}/umundo-prebuilt-${PLATFORM}.tgz \
							${UMUNDO_PREBUILT_PATH}/${VERSION}/umundo-prebuilt-${NEW_PLATFORM}.tgz
		;;
		*darwin-*-clang* )
		# gcc is ABI compatible to clang
			NEW_PLATFORM="${PLATFORM//clang/gnu}"		
			ssh ${UMUNDO_PREBUILT_HOST} \
				ln -s ${UMUNDO_PREBUILT_PATH}/${VERSION}/umundo-prebuilt-${PLATFORM}.tgz \
							${UMUNDO_PREBUILT_PATH}/${VERSION}/umundo-prebuilt-${NEW_PLATFORM}.tgz
		;;
	esac
done