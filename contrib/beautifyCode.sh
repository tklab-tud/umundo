#!/bin/bash

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

astyle  \
	--style=java \
	--indent=tab \
	--recursive \
		"${DIR}/../core/src/umundo/*.cpp" \
		"${DIR}/../core/src/umundo/*.h" \
		"${DIR}/../core/test/*.cpp" \
		"${DIR}/../rpc/src/umundo/*.cpp" \
		"${DIR}/../rpc/src/umundo/*.h" \
		"${DIR}/../rpc/test/*.cpp" \
		"${DIR}/../util/src/umundo/*.cpp" \
		"${DIR}/../util/src/umundo/*.h" \
		"${DIR}/../s11n/src/umundo/*.cpp" \
		"${DIR}/../s11n/src/umundo/*.h" \
		"${DIR}/../s11n/test/*.cpp" \
		"${DIR}/../apps/*.cpp" \
		"${DIR}/../apps/*.h"

find ${DIR}/../core/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../rpc/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../util/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../s11n/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../apps/ -iname '*.orig' -exec rm {} \;

