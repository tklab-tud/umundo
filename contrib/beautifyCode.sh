#!/bin/bash

# see http://astyle.sourceforge.net/astyle.html
# run from project root as sh ./contrib/tidy_source.sh

set -e

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

astyle  \
	--style=java \
	--indent=tab \
	--recursive "${DIR}/../core/*.cpp" "${DIR}/../core/*.h"
find ${DIR}/../core/ -iname '*.orig' -exec rm {} \;

astyle  \
	--style=java \
	--indent=tab \
	--recursive "${DIR}/../s11n/*.cpp" "${DIR}/../s11n/*.h"
find ${DIR}/../s11n/ -iname '*.orig' -exec rm {} \;

astyle  \
	--style=java \
	--indent=tab \
	--recursive "${DIR}/../rpc/*.cpp" "${DIR}/../rpc/*.h"
find ${DIR}/../rpc/ -iname '*.orig' -exec rm {} \;

astyle  \
	--style=java \
	--indent=tab \
	--recursive "${DIR}/../util/*.cpp" "${DIR}/../util/*.h"
find ${DIR}/../util/ -iname '*.orig' -exec rm {} \;

astyle  \
	--style=java \
	--indent=tab \
	--recursive "${DIR}/../apps/*.cpp" "${DIR}/../apps/*.h"
find ${DIR}/../apps/ -iname '*.orig' -exec rm {} \;
