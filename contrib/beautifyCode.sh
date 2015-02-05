#!/bin/bash

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

astyle  \
	--style=java \
	--indent=tab \
	--recursive \
		"${DIR}/../src/umundo/core/*.cpp" \
		"${DIR}/../src/umundo/core/*.h" \
		"${DIR}/../tests/core/*.cpp" \
		"${DIR}/../src/umundo/rpc/*.cpp" \
		"${DIR}/../src/umundo/rpc/*.h" \
		"${DIR}/../tests/rpc/*.cpp" \
		"${DIR}/../src/umundo/util/*.cpp" \
		"${DIR}/../src/umundo/util/*.h" \
		"${DIR}/../src/umundo/s11n/*.cpp" \
		"${DIR}/../src/umundo/s11n/*.h" \
		"${DIR}/../tests/s11n/*.cpp" \
		"${DIR}/../tools/*.cpp" \
		"${DIR}/../tools/*.h"

find ${DIR}/../src/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../tests/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../tools/ -iname '*.orig' -exec rm {} \;

