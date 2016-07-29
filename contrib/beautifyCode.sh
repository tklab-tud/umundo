#!/bin/bash

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

astyle  \
	--style=java \
	--indent=tab \
	--recursive \
		"${DIR}/../src/umundo/*.cpp" \
		"${DIR}/../src/umundo/*.h" \
		"${DIR}/../tests/*.cpp" \
		"${DIR}/../tools/*.cpp" \
		"${DIR}/../tools/*.h"

find ${DIR}/../src/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../tests/ -iname '*.orig' -exec rm {} \;
find ${DIR}/../tools/ -iname '*.orig' -exec rm {} \;

