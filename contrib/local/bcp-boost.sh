#!/bin/sh

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"
CWD=`pwd`

SOURCE_FILES=`find ${DIR}/../../core/ -name \*.h -print -o -name \*.cpp -print`
#echo ${SOURCE_FILES}

/Users/sradomski/Documents/TK/Code/boost_1_51_0/dist/bin/bcp \
--boost=/Users/sradomski/Documents/TK/Code/boost_1_51_0 \
--scan ${SOURCE_FILES} \
${DIR}/../prebuilt/include

rm -rf ${DIR}/../prebuilt/include/libs
