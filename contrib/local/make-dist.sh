#!/bin/bash

ME=`basename $0`
DIR="$( cd "$( dirname "$0" )" && pwd )"

# do not tar ._ files
export COPY_EXTENDED_ATTRIBUTES_DISABLE=1
export COPYFILE_DISABLE=1

WIN7_HOST="10.211.55.45"
PROTOBUF_PREFIX="C:\Users\sradomski\Desktop\build\protobuf\protobuf-2.6.1"

cp /Users/sradomski/Developer/SDKs/MSVC\ Redist/* ${DIR}/../../package/

############################
# Compile libraries
############################

cd ${DIR}

./remove-dsstore-files.sh

#echo -n "Run tests when after building? [y/N]: "; read BUILD_TESTS
if [ "$BUILD_TESTS" == "y" ] || [ "$BUILD_TESTS" == "Y" ]; then
	export BUILD_TESTS=ON
else
	export BUILD_TESTS=OFF
fi

# make sure to cross-compile before windows as we will copy all the files into the VMs
echo -n "Build umundo for iOS? [y/N]: "; read BUILD_IOS
if [ "$BUILD_IOS" == "y" ] || [ "$BUILD_IOS" == "Y" ]; then
  echo == BUILDING UMUNDO FOR IOS =========================================================
  ${DIR}/../build-scripts/build-umundo-ios.sh
fi

echo -n "Build umundo for Android? [y/N]: "; read BUILD_ANDROID
if [ "$BUILD_ANDROID" == "y" ] || [ "$BUILD_ANDROID" == "Y" ]; then
  echo == BUILDING UMUNDO FOR Android =========================================================
  export ANDROID_NDK=~/Developer/SDKs/android-ndk-r8b
  ${DIR}/../build-scripts/build-umundo-android.sh
fi

# echo -n "Build umundo for Raspberry Pi? [y/N]: "; read BUILD_RASPBERRY_PI
# if [ "$BUILD_RASPBERRY_PI" == "y" ] || [ "$BUILD_RASPBERRY_PI" == "Y" ]; then
#   echo "Start the Raspberry Pi system named 'raspberrypi' and press return" && read
#   echo == BUILDING UMUNDO FOR Raspberry Pi =========================================================
#   export UMUNDO_BUILD_HOST=raspberrypi
#   export UMUNDO_BUILD_ARCH=32
#   expect build-linux.expect
# fi

echo -n "Build umundo for Linux 32Bit? [y/N]: "; read BUILD_LINUX32
if [ "$BUILD_LINUX32" == "y" ] || [ "$BUILD_LINUX32" == "Y" ]; then
  echo "Start the Linux 32Bit system named 'debian' and press return" && read
  echo == BUILDING UMUNDO FOR Linux 32Bit =========================================================
  export UMUNDO_BUILD_HOST=debian
  export UMUNDO_BUILD_ARCH=32
  export JAVA_AWT_LIBRARY=/usr/lib/jvm/java-7-openjdk-i386/jre/lib/i386/libawt.so
  export JAVA_JVM_LIBRARY=/usr/lib/jvm/java-7-openjdk-i386/jre/lib/i386/server/libjvm.so

  expect build-linux.expect
fi

echo -n "Build umundo for Linux 64Bit? [y/N]: "; read BUILD_LINUX64
if [ "$BUILD_LINUX64" == "y" ] || [ "$BUILD_LINUX64" == "Y" ]; then
  echo "Start the Linux 64Bit system named 'debian64' and press return" && read
  echo == BUILDING UMUNDO FOR Linux 64Bit =========================================================
  export UMUNDO_BUILD_HOST=debian64
  export UMUNDO_BUILD_ARCH=64
  export JAVA_AWT_LIBRARY=/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/amd64/libawt.so
  export JAVA_JVM_LIBRARY=/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/amd64/server/libjvm.so
  expect build-linux.expect
fi

echo -n "Build umundo for Windows 32Bit MSVC1600? [y/N]: "; read BUILD_WIN32_MSVC1600
if [ "$BUILD_WIN32_MSVC1600" == "y" ] || [ "$BUILD_WIN32_MSVC1600" == "Y" ]; then
  echo "Start the Windows 64Bit system named '${WIN7_HOST}' and press return" && read
  export UMUNDO_BUILD_HOST=${WIN7_HOST}

  # VS2010 32Bit x86
  # C:\Windows\System32\cmd.exe /k ""C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"" x86
  echo == BUILDING UMUNDO FOR Windows 32Bit MSVC1600 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat\" x86"
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1600-32\""
  export UMUNDO_BUILD_ARCH=32
  export UMUNDO_COMPILER_VER=1600
  TERM=xterm expect build-windows.expect
fi

echo -n "Build umundo for Windows 64Bit MSVC1600? [y/N]: "; read BUILD_WIN64_MSVC1600

if [ "$BUILD_WIN64_MSVC1600" == "y" ] || [ "$BUILD_WIN64_MSVC1600" == "Y" ]; then
  echo "Start the Windows 64Bit system named '${WIN7_HOST}' and press return" && read
  export UMUNDO_BUILD_HOST=${WIN7_HOST}

  # VS2010 64Bit amd64
  # C:\Windows\System32\cmd.exe /k ""C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"" amd64
  echo == BUILDING UMUNDO FOR Windows 64Bit MSVC1600 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat\" amd64"
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1600-64\""
  export UMUNDO_BUILD_ARCH=64
  export UMUNDO_COMPILER_VER=1600
  TERM=xterm expect build-windows.expect
fi

echo -n "Build umundo for Windows 32Bit MSVC1800? [y/N]: "; read BUILD_WIN32_MSVC1800
if [ "$BUILD_WIN32_MSVC1800" == "y" ] || [ "$BUILD_WIN32_MSVC1800" == "Y" ]; then
  echo "Start the Windows 64Bit system named '${WIN7_HOST}' and press return" && read
  export UMUNDO_BUILD_HOST=${WIN7_HOST}
  
  # VS2013 32Bit x86
  # %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"" x86
  echo == BUILDING UMUNDO FOR Windows 32Bit MSVC1800 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat\" x86"
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1800-32\""
  export UMUNDO_BUILD_ARCH=32
  export UMUNDO_COMPILER_VER=1800
  TERM=xterm expect build-windows.expect
fi

echo -n "Build umundo for Windows 64Bit MSVC1800? [y/N]: "; read BUILD_WIN64_MSVC1800
if [ "$BUILD_WIN64_MSVC1800" == "y" ] || [ "$BUILD_WIN64_MSVC1800" == "Y" ]; then
  echo "Start the Windows 64Bit system named '${WIN7_HOST}' and press return" && read
  export UMUNDO_BUILD_HOST=${WIN7_HOST}
  
  # VS2013 64Bit amd64
  # %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"" amd64
  echo == BUILDING UMUNDO FOR Windows 64Bit MSVC1800 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat\" amd64"
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1800-64\""
  export UMUNDO_BUILD_ARCH=64
  export UMUNDO_COMPILER_VER=1800
  TERM=xterm expect build-windows.expect
fi

echo -n "Build umundo for Mac OSX with libstdc++? [y/N]: "; read BUILD_MAC_LIBSTDCPP
if [ "$BUILD_MAC_LIBSTDCPP" == "y" ] || [ "$BUILD_MAC_LIBSTDCPP" == "Y" ]; then
  echo "Start the Mountain Lion system named 'epikur.ml' and press return" && read
  echo == BUILDING UMUNDO FOR Mac OSX libstdc++ ===========================================
  export UMUNDO_BUILD_HOST=epikur-ml
  expect build-macosx.expect
  
fi

echo -n "Build umundo for Mac OSX with libc++? [y/N]: "; read BUILD_MAC_LIBCPP
if [ "$BUILD_MAC_LIBCPP" == "y" ] || [ "$BUILD_MAC_LIBCPP" == "Y" ]; then
  echo == BUILDING UMUNDO FOR Mac OSX libc++ ===========================================

  export UMUNDO_BUILD_HOST=epikur
  expect build-macosx.expect
fi


############################
# Create installers
############################

echo -n "Build packages for those platforms? [a/y/N]: "; read BUILD_PACKAGES

cd ${DIR}

if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
  echo -n "Package umundo for Linux 32Bit? [y/N]: "; read BUILD_LINUX32 
fi
if [ "$BUILD_LINUX32" == "y" ] || [ "$BUILD_LINUX32" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
  echo Start the Linux 32Bit system named 'debian' again && read
  echo == PACKAGING UMUNDO FOR Linux 32Bit =========================================================
  export UMUNDO_BUILD_HOST=debian
  export UMUNDO_BUILD_ARCH=32
  export JAVA_AWT_LIBRARY=/usr/lib/jvm/java-7-openjdk-i386/jre/lib/i386/libawt.so
  export JAVA_JVM_LIBRARY=/usr/lib/jvm/java-7-openjdk-i386/jre/lib/i386/server/libjvm.so
  expect package-linux.expect
fi

if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
  echo -n "Package umundo for Linux 64Bit? [y/N]: "; read BUILD_LINUX64 
fi
if [ "$BUILD_LINUX64" == "y" ] || [ "$BUILD_LINUX64" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
  echo Start the Linux 64Bit system named 'debian64' again && read
  echo == PACKAGING UMUNDO FOR Linux 64Bit =========================================================
  export UMUNDO_BUILD_HOST=debian64
  export UMUNDO_BUILD_ARCH=64
  export JAVA_AWT_LIBRARY=/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/amd64/libawt.so
  export JAVA_JVM_LIBRARY=/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/amd64/server/libjvm.so
  expect package-linux.expect
fi

# if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
#  echo -n "Package umundo for Raspberry Pi? [y/N]: "; read BUILD_RASPBERRY_PI 
# fi
# if [ "$BUILD_RASPBERRY_PI" == "y" ] || [ "$BUILD_RASPBERRY_PI" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
#   echo Start the Raspberry Pi system named 'raspberrypi' again && read
#   echo == PACKAGING UMUNDO FOR Raspberry Pi =========================================================
#   export UMUNDO_BUILD_HOST=raspberrypi
#   export UMUNDO_BUILD_ARCH=32
#   expect package-linux.expect
# fi

export UMUNDO_BUILD_HOST=${WIN7_HOST}

if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
 echo -n "Package umundo for Windows 32Bit MSVC1600? [y/N]: "; read BUILD_WIN32_MSVC1600
fi
if [ "$BUILD_WIN32_MSVC1600" == "y" ] || [ "$BUILD_WIN32_MSVC1600" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
  echo Start the Windows 64Bit system named ${WIN7_HOST} again && read
  export ANT_BINARY="\"C:\Program Files\apache-ant-1.8.4\bin\ant\""
    
  # VS2010 32Bit x86
  # C:\Windows\System32\cmd.exe /k ""C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"" x86
  echo == PACKAGING UMUNDO FOR Windows 32Bit MSVC1600 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat\" x86"
  export UMUNDO_BUILD_ARCH=32
  export UMUNDO_COMPILER_VER=1600
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1600-32\""
  export UMUNDO_OUT_DEST="C:\Users\sradomski\umundo\package\windows-x86\msvc-1600"
  TERM=xterm expect package-windows.expect

fi
if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
 echo -n "Package umundo for Windows 64Bit MSVC1600? [y/N]: "; read BUILD_WIN64_MSVC1600
fi
if [ "$BUILD_WIN64_MSVC1600" == "y" ] || [ "$BUILD_WIN64_MSVC1600" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then


  # VS2010 64Bit amd64
  # C:\Windows\System32\cmd.exe /k ""C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"" amd64
  echo == PACKAGING UMUNDO FOR Windows 64Bit MSVC1600 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat\" amd64"
  export UMUNDO_BUILD_ARCH=64
  export UMUNDO_COMPILER_VER=1600
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1600-64\""
  export UMUNDO_OUT_DEST="C:\Users\sradomski\umundo\package\windows-x86_64\msvc-1600"
  TERM=xterm expect package-windows.expect
  
fi
if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
 echo -n "Package umundo for Windows 32Bit MSVC1800? [y/N]: "; read BUILD_WIN32_MSVC1800
fi
if [ "$BUILD_WIN32_MSVC1800" == "y" ] || [ "$BUILD_WIN32_MSVC1800" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then

  # # VS2013 32Bit x86
  # # %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"" x86
  echo == PACKAGING UMUNDO FOR Windows 32Bit MSVC1800 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat\" x86"
  export UMUNDO_BUILD_ARCH=32
  export UMUNDO_COMPILER_VER=1800
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1800-32\""
  export UMUNDO_OUT_DEST="C:\Users\sradomski\umundo\package\windows-x86\msvc-1800"
  TERM=xterm expect package-windows.expect
  
fi
if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
 echo -n "Package umundo for Windows 64Bit MSVC1800? [y/N]: "; read BUILD_WIN64_MSVC1800
fi
if [ "$BUILD_WIN64_MSVC1800" == "y" ] || [ "$BUILD_WIN64_MSVC1800" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
  # # VS2013 64Bit amd64
  # # %comspec% /k ""C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"" amd64
  echo == PACKAGING UMUNDO FOR Windows 64Bit MSVC1800 ===============================================
  export UMUNDO_VCVARSALL="\"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat\" amd64"
  export UMUNDO_BUILD_ARCH=64
  export UMUNDO_COMPILER_VER=1800
  export UMUNDO_PROTOBUF_ROOT="\"${PROTOBUF_PREFIX}-1800-64\""
  export UMUNDO_OUT_DEST="C:\Users\sradomski\umundo\package\windows-x86_64\msvc-1800"
  TERM=xterm expect package-windows.expect
fi

if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
  echo -n "Package umundo for MacOSX with libstdc++? [y/N]: "; read BUILD_MAC_LIBSTDCPP 
fi
if [ "$BUILD_MAC_LIBSTDCPP" == "y" ] || [ "$BUILD_MAC_LIBSTDCPP" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then
  echo Start the MacOSX system named 'epikur-ml' again && read
  echo == PACKAGING UMUNDO FOR Mac OSX libstdc++ ===========================================
  export UMUNDO_BUILD_HOST=epikur-ml
  expect package-macosx.expect
  
fi

if [ "$BUILD_PACKAGES" == "n" ] || [ "$BUILD_PACKAGES" == "N" ] || [ "$BUILD_PACKAGES" == "" ]; then
  echo -n "Package umundo for MacOSX with libc++? [y/N]: "; read BUILD_MAC_LIBCPP 
fi
if [ "$BUILD_MAC_LIBCPP" == "y" ] || [ "$BUILD_MAC_LIBCPP" == "Y" ] || [ "$BUILD_PACKAGES" == "a" ]; then

  echo == PACKAGING UMUNDO FOR Mac OSX libc++ ===========================================
  export UMUNDO_BUILD_HOST=epikur
  expect package-macosx.expect
fi

############################
# Validate installers
############################

#./validate-installers.pl ${DIR}/../../installer
