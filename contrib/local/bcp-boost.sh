#!/bin/sh

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

SOURCE_FILES=`find ${DIR}/../../ -name \*.h -print -o -name \*.cpp -print`
#echo ${SOURCE_FILES}

/Users/sradomski/Documents/TK/Code/boost_1_61_0/dist/bin/bcp \
--boost=/Users/sradomski/Documents/TK/Code/boost_1_61_0 \
--scan ${SOURCE_FILES} \
${DIR}/../src

rm -rf ${DIR}/../prebuilt/include/libs
