#
# To cross compile, choose a toolchain file (e.g. iOS):
# build$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../contrib/cmake/CrossCompile-iOS.cmake
#

# we need 2.8.9 for java but travis only features 2.8.7
# TODO: give a sensible error message when building the jar fails
cmake_minimum_required(VERSION 2.8.7)

cmake_policy(PUSH)
IF((CMAKE_VERSION VERSION_EQUAL "2.8.12") OR (CMAKE_VERSION VERSION_GREATER "2.8.12"))
	# List public interface libraries for a shared library or executable
	CMAKE_POLICY(SET CMP0022 OLD) 
ENDIF() 
if(POLICY CMP0042)
	# MACOSX_RPATH is enabled by default
	cmake_policy(SET CMP0042 OLD)
endif()
if(POLICY CMP0045)
	# Error on non-existent target in get_target_property
	cmake_policy(SET CMP0045 OLD)
endif()
if(POLICY CMP0046)
	# Error on non-existent dependency in add_dependencies
	cmake_policy(SET CMP0046 OLD)
endif()
if(POLICY CMP0026)
	# Disallow use of the LOCATION target property
	cmake_policy(SET CMP0026 OLD)
endif()
if(POLICY CMP0038)
	# Targets may not link directly to themselves
	cmake_policy(SET CMP0038 OLD)
endif()
if(POLICY CMP0054)
	# Escape variables in if
	cmake_policy(SET CMP0054 OLD)
endif()

# specify umundo version
SET(UMUNDO_VERSION_MAJOR "0")
SET(UMUNDO_VERSION_MINOR "6")
SET(UMUNDO_VERSION_PATCH "0")
SET(UMUNDO_VERSION "${UMUNDO_VERSION_MAJOR}.${UMUNDO_VERSION_MINOR}.${UMUNDO_VERSION_PATCH}")

# build type has to be set before the project definition
SET(BUILD_TYPE_HELP "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug, Release, RelWithDebInfo, MinSizeRel.")
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING ${BUILD_TYPE_HELP})
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING ${BUILD_TYPE_HELP})
ENDIF()

project(umundo)

# where to find the cmake modules we distribute
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/contrib/cmake)
include("${CMAKE_MODULE_PATH}/UMUNDOMacros.cmake")

# CMake 2.8.11 reports AMD64 for Windows 64Bit, where earlier versions reported x86
# we resolve it with a 64bit check later
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(CMAKE_SYSTEM_PROCESSOR "x86")
endif()
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i386")
  set(CMAKE_SYSTEM_PROCESSOR "x86")
endif()
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
  set(CMAKE_SYSTEM_PROCESSOR "x86")
endif()
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(CMAKE_SYSTEM_PROCESSOR "x86")
endif()
if (NOT CMAKE_SYSTEM_PROCESSOR)
	# this happens when we ssh into windows to build there
	set(CMAKE_SYSTEM_PROCESSOR "x86")
endif()

# is this a 64Bit host?
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(HOST_64BIT ON)
	set(64BIT_LIB_POSTFIX 64)
	set(64BIT_SUFFIX "_64")
	# additional library paths to be searched
	set(FIND_LIBRARY_USE_LIB64_PATHS ON)
# 	LIST(APPEND CMAKE_PREFIX_PATH "/usr/local/lib64")
# 	LIST(APPEND CMAKE_PREFIX_PATH "/usr/lib64")
endif()

# lower case version of system name and compiler for library paths
string(TOLOWER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME_LC)
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} CMAKE_CXX_COMPILER_ID_LC)

if (WIN32)
	set(CMAKE_CXX_COMPILER_ID_LC ${CMAKE_CXX_COMPILER_ID_LC}${MSVC_VERSION})
endif()

# use folders in the IDEs for the various targets (e.g. Library, Testing, Tools ..)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################
# Configuration of library type, linking and language bindings
################################################################

OPTION(BUILD_CONVENIENCE_LIB  "Build umundo convenience library" OFF)
OPTION(BUILD_BINDINGS         "Build umundo language bindings" ON)
OPTION(DIST_PREPARE           "Put libraries into the lib folder of the source tree" OFF)
OPTION(BUILD_WITH_CXX11       "Build with C++11" OFF)
OPTION(ENABLE_TRACING         "Log functions as they are entered" OFF)
if (ENABLE_TRACING)
	add_definitions("-DENABLE_TRACING")
endif()

# setup defaults
if (CMAKE_CROSSCOMPILING)
	OPTION(BUILD_UMUNDO_TOOLS "Build tools" OFF)
	OPTION(BUILD_TESTS "Build tests" OFF)	
else()
	OPTION(BUILD_UMUNDO_TOOLS "Build tools" ON)
	OPTION(BUILD_TESTS "Build tests" ON)	
endif()

if (WIN32 OR CMAKE_CROSSCOMPILING)
	OPTION(BUILD_SHARED_LIBS "Build shared libraries" OFF)
else()
	OPTION(BUILD_SHARED_LIBS "Build shared libraries" ON)
endif()

# prefer static libraries for linking
set(CMAKE_FIND_LIBRARY_SUFFIXES_SHARED ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES_STATIC .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

if(BUILD_PREFER_STATIC_LIBRARIES)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_STATIC})
endif()
set(CMAKE_FIND_LIBRARY_SUFFIXES_ORIG ${CMAKE_FIND_LIBRARY_SUFFIXES})

# Note: We avoid linking against static libraries on 64Bit due to the -fPIC issues, see:
# http://www.gentoo.org/proj/en/base/amd64/howtos/index.xml?part=1&chap=3
if (NOT HOST_64BIT AND CMAKE_CROSSCOMPILING OR WIN32)
	OPTION(BUILD_PREFER_STATIC_LIBRARIES "Prefer static libraries when resolving dependencies" ON)
else()
	OPTION(BUILD_PREFER_STATIC_LIBRARIES "Prefer static libraries when resolving dependencies" OFF)
endif()
OPTION(DEBUG_THREADS "Print log messages on mutex and monitor access" OFF)
if (DEBUG_THREADS)
	add_definitions("-DDEBUG_THREADS")
endif()

############################################################
# Search paths for cross compiling and prebuilds
############################################################

if (APPLE)
	set(LIBCPP_NAME "libstdc++") # before mavericks
	# get MacOSX version
	execute_process(COMMAND /usr/bin/sw_vers -productVersion
		OUTPUT_VARIABLE MACOSX_VERSION
		ERROR_VARIABLE MACOSX_VERSION_errors
		RESULT_VARIABLE MACOSX_VERSION_result
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	if (MACOSX_VERSION)
		THREE_PART_VERSION_TO_VARS(
			${MACOSX_VERSION}
			MACOSX_VERSION_MAJOR 
			MACOSX_VERSION_MINOR 
			MACOSX_VERSION_PATCH)
	endif()
	if (MACOSX_VERSION VERSION_GREATER "10.8.99")
		set(LIBCPP_NAME "libc++") # mavericks
		# LIST(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})
		# make sure that we find libxml2 here first
		set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT} ${CMAKE_FIND_ROOT_PATH})
	endif()
	if (MACOSX_VERSION VERSION_GREATER "10.9.99")
		if (NOT CMAKE_SIZEOF_VOID_P) # yosemite with cmake 3.0.2!
			set(CMAKE_SIZEOF_VOID_P 8) 
		endif()
	endif()
endif()


# We use the toolchain file from http://code.google.com/p/android-cmake/
if (CMAKE_CROSSCOMPILING AND ANDROID_ABI)
	set(ANDROID ON)
	set(CMAKE_CROSSCOMPILING_TARGET android)
	set(CMAKE_SYSTEM_VERSION ${ANDROID_NATIVE_API_LEVEL})
endif()

#
# Setting the CMAKE_PREFIX_PATH to a list of directories will cause all CMake modules
# to look in these directories in addition to the system search paths:
# http://www.vtk.org/Wiki/CMake_Cross_Compiling#Searching_and_finding_external_software
#

#
# determine platform id
#
SET(UMUNDO_PLATFORM_ID)
if (CMAKE_SYSTEM_NAME_LC)
	set(UMUNDO_PLATFORM_ID ${CMAKE_SYSTEM_NAME_LC})
else()
	set(UMUNDO_PLATFORM_ID "unknown")
endif()

if (CMAKE_SYSTEM_PROCESSOR)
	set(UMUNDO_PLATFORM_ID "${UMUNDO_PLATFORM_ID}-${CMAKE_SYSTEM_PROCESSOR}${64BIT_SUFFIX}")
else()
	set(UMUNDO_PLATFORM_ID "unknown")
endif()

if (CMAKE_CXX_COMPILER_ID_LC)
	set(UMUNDO_PLATFORM_ID "${UMUNDO_PLATFORM_ID}-${CMAKE_CXX_COMPILER_ID_LC}")
endif()

if (LIBCPP_NAME)
	set(UMUNDO_PLATFORM_ID "${UMUNDO_PLATFORM_ID}-${LIBCPP_NAME}")
endif()


# handle cross compiles
if (CMAKE_CROSSCOMPILING)
	if (IOS)
		SET(UMUNDO_PLATFORM_ID "ios-universal")
		SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "BOTH")
	elseif (ANDROID)
		# handle mips, x86 and arm
		SET(UMUNDO_PLATFORM_ID "android-${ANDROID_ABI}")
		
		SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "BOTH") # we need both to find jni - we don't?
		SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM "BOTH")
		SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "BOTH")
		
		LIST(APPEND CMAKE_PREFIX_PATH "/usr/local/bin") # this is needed to find swig
		LIST(APPEND CMAKE_PREFIX_PATH "/opt/local/bin") # this is needed to find protoc
	endif()
endif()

#message(FATAL_ERROR "UMUNDO_PLATFORM_ID: ${UMUNDO_PLATFORM_ID}")

# prepend some directories given at cmake invocation time
if (CMAKE_CUSTOM_FIND_PATH)
	SET(CMAKE_PREFIX_PATH "${CMAKE_CUSTOM_FIND_PATH};${CMAKE_PREFIX_PATH}")
endif()

#message(FATAL_ERROR "CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")

message(STATUS "Searching for prebuilt libraries in: ${CMAKE_PREFIX_PATH}")

if (WIN32)
	include_directories(${PROJECT_SOURCE_DIR}/contrib/src/xgetopt)
endif()

SET(BUILD_WITH_COMPRESSION_MINIZ ON CACHE BOOL "Enable message miniz compression")
SET(BUILD_WITH_COMPRESSION_FASTLZ ON CACHE BOOL "Enable message fastlz compression")
SET(BUILD_WITH_COMPRESSION_LZ4 ON CACHE BOOL "Enable message lz4 compression")
if (BUILD_WITH_COMPRESSION_MINIZ)
	SET( BUILD_WITH_COMPRESSION_LEVEL_MINIZ 5)
	include_directories(${PROJECT_SOURCE_DIR}/contrib/src/miniz)
endif()
if(BUILD_WITH_COMPRESSION_FASTLZ)
	include_directories(${PROJECT_SOURCE_DIR}/contrib/src/fastlz)
endif()
if(BUILD_WITH_COMPRESSION_LZ4)
	SET( BUILD_WITH_COMPRESSION_LEVEL_LZ4 0)
	include_directories(${PROJECT_SOURCE_DIR}/contrib/src/lz4)
endif()
# 

############################################################
# General setup
############################################################

# a dummy target to depend on the targets needed for tests, see:
# http://stackoverflow.com/questions/733475/cmake-ctest-make-test-doesnt-build-tests
add_custom_target(ALL_TESTS COMMENT "Building all tests when BUILD_TESTS is enabled")
if (BUILD_TESTS)
	enable_testing()
	SET(ENV{CTEST_OUTPUT_ON_FAILURE} ON)
endif()

# some compiler flags

# http://ptspts.blogspot.de/2013/12/how-to-make-smaller-c-and-c-binaries.html
# https://blog.algolia.com/android-ndk-how-to-reduce-libs-size/

# CHECK_AND_ADD_CXX_COMPILER_FLAG("-fno-rtti") # needed for wrappers

# CHECK_AND_ADD_CXX_COMPILER_FLAG("-fvtable-gc") # clang barfs

if (NOT MSVC)
	CHECK_AND_ADD_COMPILER_FLAG("-fno-stack-protector")
	CHECK_AND_ADD_COMPILER_FLAG("-fomit-frame-pointer")
	# CHECK_AND_ADD_COMPILER_FLAG("-ffunction-sections -fdata-sections -Wl,--gc-sections") # clang barfs

	if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86" AND NOT HOST_64BIT)
		CHECK_AND_ADD_CXX_COMPILER_FLAG("-mpreferred-stack-boundary=2")
		CHECK_AND_ADD_CXX_COMPILER_FLAG("-falign-functions=1 -falign-jumps=1 -falign-loops=1")
	endif()

	CHECK_AND_ADD_C_COMPILER_FLAG("-fno-unwind-tables")
	CHECK_AND_ADD_C_COMPILER_FLAG("-fno-asynchronous-unwind-tables")

	CHECK_AND_ADD_COMPILER_FLAG("-fno-math-errno")
	CHECK_AND_ADD_COMPILER_FLAG("-fno-unroll-loops")
	CHECK_AND_ADD_COMPILER_FLAG("-fmerge-all-constants")
	CHECK_AND_ADD_COMPILER_FLAG("-fno-ident")
	# CHECK_AND_ADD_COMPILER_FLAG("-mfpmath=387") # clang barfs
	# CHECK_AND_ADD_COMPILER_FLAG("-mfancy-math-387") # clang barfs
	# CHECK_AND_ADD_COMPILER_FLAG("-fshort-double") # clang barfs
	# CHECK_AND_ADD_COMPILER_FLAG("-fsingle-precision-constant") # clang barfs
	# CHECK_AND_ADD_COMPILER_FLAG("-ffast-math") # clang barfs

	# CHECK_AND_ADD_COMPILER_FLAG("-Wl,-z,norelro") # clang barfs
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)

elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)

elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	add_definitions("-D_SCL_SECURE_NO_WARNINGS")
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
else()
	message(STATUS "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()
set(CMAKE_COMPILER_STRING "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")


if (BUILD_WITH_CXX11)
	# http://stackoverflow.com/a/25836953/990120
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11_FLAG)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X_FLAG)
	if(COMPILER_SUPPORTS_CXX11_FLAG)
	    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X_FLAG)
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	endif()

	# https://cmake.org/Bug/view.php?id=15361
	set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_CXX11_STANDARD_COMPILE_OPTION})

	include(CheckCXXSourceCompiles)
	check_cxx_source_compiles("
		#include <thread>
		int main(){ return 0; }
	" CXX_HAS_THREADS)
	check_cxx_source_compiles("
		#include <memory>
		int main(){ std::shared_ptr<void> foo(NULL); return 0; }
	" CXX_HAS_SHARED_PTR)

	set(CXX_MISSING_FEATURES "")
	if(NOT CXX_HAS_THREADS)
		set(CXX_MISSING_FEATURES "${CXX_MISSING_FEATURES} threads")
	endif()
	if(NOT CXX_HAS_SHARED_PTR)
		set(CXX_MISSING_FEATURES "${CXX_MISSING_FEATURES} shared_ptr")
	endif()

	if (CXX_MISSING_FEATURES)
		message(FATAL_ERROR "Compiler is missing required C++11 features: ${CXX_MISSING_FEATURES}")
	endif()
	add_definitions("-DWITH_CXX11")
endif()

if (CMAKE_CROSSCOMPILING OR WIN32)
	OPTION(BUILD_SHARED_LIBS "Build shared libraries" OFF)
else()
	OPTION(BUILD_SHARED_LIBS "Build shared libraries" ON)
endif()

# see http://www.mail-archive.com/cmake@cmake.org/msg23240.html
if (APPLE AND OFF)
#	add_definitions("-D_DARWIN_UNLIMITED_SELECT")
	if (HOST_64BIT)
		# do not build universal binaries anymore ..
		# set(CMAKE_OSX_ARCHITECTURES "i386")
	endif()
  if (MACOSX_VERSION VERSION_LESS 10.9)
    # forces libstc++
  	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)
    set(LIBCPP libstdc++)
  else()
    # forces libc++
  	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
    set(LIBCPP libc++)
  endif()
  if (MACOSX_VERSION VERSION_GREATER 10.6.99) 
		# unrecognized command line option in 10.6 
		foreach(FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
				set(${FLAGS} "${${FLAGS}} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -stdlib=${LIBCPP}")
	  endforeach()
	endif()
endif()

if (IOS)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 4.3)
	foreach(FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
			set(${FLAGS} "${${FLAGS}} -miphoneos-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
  endforeach()
endif()

# build type as macro to strip logging in release builds
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
add_definitions("-DBUILD_${BUILD_TYPE}")


# Enable verbose Makefiles to debug the build process itself
#set(CMAKE_VERBOSE_MAKEFILE OFF)

if (CMAKE_BUILD_TYPE MATCHES Release)
	SET(LOGLEVEL_COMMON "1" CACHE STRING "Log-level for common - higher is more verbose" FORCE)
	SET(LOGLEVEL_DISC "1" CACHE STRING "Log-level for discovery - higher is more verbose" FORCE)
	SET(LOGLEVEL_NET "1" CACHE STRING "Log-level for networking - higher is more verbose" FORCE)
else()
	SET(LOGLEVEL_COMMON "2" CACHE STRING "Log-level for common - higher is more verbose" FORCE)
	SET(LOGLEVEL_DISC "2" CACHE STRING "Log-level for discovery - higher is more verbose" FORCE)
	SET(LOGLEVEL_NET "2" CACHE STRING "Log-level for networking - higher is more verbose" FORCE)
endif()

SET_PROPERTY(CACHE LOGLEVEL_COMMON PROPERTY STRINGS 0 1 2 3)
SET_PROPERTY(CACHE LOGLEVEL_DISC PROPERTY STRINGS 0 1 2 3)
SET_PROPERTY(CACHE LOGLEVEL_NET PROPERTY STRINGS 0 1 2 3)

############################################################
# postfixes for all built targets depending on build type
############################################################

SET(CMAKE_DEBUG_POSTFIX "${64BIT_LIB_POSTFIX}_d")
SET(CMAKE_RELEASE_POSTFIX "${64BIT_LIB_POSTFIX}")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "${64BIT_LIB_POSTFIX}_rd")
SET(CMAKE_MINSIZEREL_POSTFIX "${64BIT_LIB_POSTFIX}_s")

SET(CMAKE_LIBRARY_POSTFIX ${CMAKE_${BUILD_TYPE}_POSTFIX})
# message(FATAL_ERROR "CMAKE_LIBRARY_POSTFIX: ${CMAKE_LIBRARY_POSTFIX}")

############################################################
# Select implementations depending on build target
############################################################

# select implementations for discovery and network connectivity
if(UNIX AND NOT APPLE AND NOT IOS AND NOT ANDROID)
	# we used to build with avahi on linux but there is a bug and the ML is dead
	OPTION(DISC_BONJOUR "Use bonjour for discovery" ON)
	OPTION(DISC_BONJOUR_EMBED "Embed mDNS discovery service" ON)
	OPTION(DISC_AVAHI "Use avahi for discovery" OFF)
	OPTION(DISC_BROADCAST "Use Broadcast discovery" ON)
else()
	# build with bonjour everywhere else
	OPTION(DISC_BONJOUR "Use bonjour for discovery" ON)
	OPTION(DISC_AVAHI "Use avahi for discovery" OFF)
	OPTION(DISC_BROADCAST "Use Broadcast discovery" ON)
	if(CMAKE_CROSSCOMPILING AND ANDROID)
		OPTION(DISC_BONJOUR_EMBED "Embed mDNS discovery service" ON)
		# required in patched bonjour headers
		add_definitions("-DTARGET_OS_ANDROID")
	elseif(WIN32)
		# build with embedded bonjour until deployment situation of bonjour improves
		OPTION(DISC_BONJOUR_EMBED "Embed mDNS discovery service" ON)
	else()
		OPTION(DISC_BONJOUR_EMBED "Embed mDNS discovery service" OFF)
	endif()
endif()
OPTION(NET_RTP "Allow pub/sub via RTP" ON)

# CMake does not allow explicit dependencies
if (DISC_BONJOUR_EMBED AND NOT DISC_BONJOUR)
	set(DISC_BONJOUR ON)
endif()
if (DISC_BONJOUR AND DISC_AVAHI)
	message(FATAL_ERROR "Cannot use both avahi and bonjour for discovery - choose either DISC_BONJOUR or DISC_AVAHI!")
endif()
if (NOT DISC_AVAHI AND NOT DISC_BONJOUR)
	message(FATAL_ERROR "No discovery implementation choosen - enable either DISC_BONJOUR or DISC_AVAHI!")
endif()

if(WIN32)
	SET(THREAD_WIN32 ON)
else()
	SET(THREAD_PTHREAD ON)
endif()

OPTION(NET_ZEROMQ "Use 0MQ for networking" ON)

if (NET_ZEROMQ)
	SET(NET_ZEROMQ_SND_HWM "300000" CACHE STRING "Maximum queue size for publishers")
	SET(NET_ZEROMQ_RCV_HWM "300000" CACHE STRING "Maximum queue size for subscribers")
endif()

############################################################
# Library location, type and linking
############################################################


# built shared or static libraries?
if(BUILD_SHARED_LIBS)
	add_definitions("-DCOMPILING_DLL")
	# set(BUILD_CONVENIENCE_LIB OFF)
else()
	add_definitions("-DUMUNDO_STATIC")
	# set(BUILD_CONVENIENCE_LIB ON)
endif()

# where should libraries end up?
if (DIST_PREPARE)
	if (CMAKE_CROSSCOMPILING)
		if(IOS)
			set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/package/cross-compiled/ios)
		elseif(ANDROID)
			set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/package/cross-compiled/android/${ANDROID_ABI})
		else()
			set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/package/cross-compiled/${UMUNDO_PLATFORM_ID})
		endif()
	else()
		set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/package/${UMUNDO_PLATFORM_ID})
	endif()
else()
	set(OUTPUT_DIR ${PROJECT_BINARY_DIR})
endif()

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}/bin" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}/lib" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}/lib" )
foreach( OUTPUT_CONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUT_CONFIG} OUTPUT_CONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} "${OUTPUT_DIR}/bin" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} "${OUTPUT_DIR}/lib" )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUT_CONFIG} "${OUTPUT_DIR}/lib" )
endforeach()

include_directories(src)
include_directories(contrib/src)

############################################################
# Find all our dependant libraries here so we can control
# their link type in a central place
############################################################

if (BUILD_PREFER_STATIC_LIBRARIES)
	OPTION(BUILD_PREFER_STATIC_ZEROMQ "Prefer static ZeroMQ libraries when linking" ON)
	OPTION(BUILD_PREFER_STATIC_RE "Prefer static RE libraries when linking" ON)
else()
	OPTION(BUILD_PREFER_STATIC_ZEROMQ "Prefer static ZeroMQ libraries when linking" OFF)
	OPTION(BUILD_PREFER_STATIC_RE "Prefer static RE libraries when linking" OFF)
endif()


#
# Download and build dependent libraries
#

# libsquash
# include("${CMAKE_MODULE_PATH}/BuildSquash.cmake")
# message(STATUS "Did not look for Squash on your system")
# message(STATUS "--> will download and compile")
# if (LIBSQUASH_BUILT)
# 	include_directories(${LIBSQUASH_INCLUDE_DIR})
# 	list (APPEND USCXML_CORE_LIBS ${LIBSQUASH_LIBRARY})
# 	list (APPEND USCXML_PREREQS libsquash)
# else()
# 	message(FATAL_ERROR "Could not build libsquash")
# endif()

# Bonjour
if (DISC_BONJOUR)
	find_package(Bonjour)
	if (Bonjour_FOUND)
		include_directories(${Bonjour_INCLUDE_DIR})
		LIST(APPEND UMUNDO_LIBRARIES ${Bonjour_LIBRARY})
	else()
		if (DISC_BONJOUR_EMBED)
			include("${CMAKE_MODULE_PATH}/BuildBonjour.cmake")
			message(STATUS "Did not find Bonjour")
			message(STATUS "--> will download and compile")
			if (BONJOUR_BUILT)
				include_directories(${BONJOUR_INCLUDE_DIR})
				list (APPEND UMUNDO_LIBRARIES ${BONJOUR_LIBRARY})
				list (APPEND UMUNDO_PREREQS bonjour)
			else()
				message(FATAL_ERROR "Could not build bonjour")
			endif()
		else()
			if (NOT APPLE AND NOT IOS)
				message(FATAL_ERROR "Could not find bonjour")
			endif()
		endif()
	endif()
endif()

# ZeroMQ
if(NET_ZEROMQ)
	find_package(ZeroMQ)
	if (ZeroMQ_FOUND)
		include_directories(${ZeroMQ_INCLUDE_DIR})
		list (APPEND UMUNDO_LIBRARIES ${ZeroMQ_LIBRARY})
	else()
		include("${CMAKE_MODULE_PATH}/BuildLibZMQ.cmake")
		message(STATUS "Did not find ZeroMQ")
		message(STATUS "--> will download and compile")
		if (LIBZMQ_BUILT)
			include_directories(${ZeroMQ_INCLUDE_DIR})
			list (APPEND UMUNDO_LIBRARIES ${LIBZMQ_LIBRARY})
			list (APPEND UMUNDO_PREREQS libzmq)
		else()
			message(FATAL_ERROR "Could not build ZeroMQ")
		endif()
	endif()
endif()

# RE
if (NET_RTP)
	include("${CMAKE_MODULE_PATH}/BuildLibRE.cmake")
	message(STATUS "Did not look for LibRE")
	message(STATUS "--> will download and compile")
	if (LIBRE_BUILT)
		include_directories(${LIBRE_INCLUDE_DIR})
		list (APPEND UMUNDO_LIBRARIES ${LIBRE_LIBRARY})
		list (APPEND UMUNDO_PREREQS libre)
	else()
		message(FATAL_ERROR "Could not build libre")
	endif()
	if (ANDROID_ABI)	
		add_definitions("-D__ssize_t_defined")
	endif()
endif()

if(DISC_AVAHI)
	find_package(Avahi REQUIRED)
	LIST(APPEND UMUNDO_LIBRARIES ${Avahi_LIBRARIES})
	include_directories(${Avahi_INCLUDE_DIR})
endif()

include(CheckIncludeFile)
CHECK_INCLUDE_FILE(inttypes.h HAVE_INTTYPES_H)
if (HAVE_INTTYPES_H AND NOT WIN32)
	add_definitions("-DHAVE_INTTYPES_H")
endif()

CHECK_INCLUDE_FILE(stdbool.h HAVE_STDBOOL_H)
if (HAVE_STDBOOL_H)
	add_definitions("-DHAVE_STDBOOL_H")
endif()


# Mosquitto
# include("${CMAKE_MODULE_PATH}/BuildMosquitto.cmake")
# message(STATUS "Did not look for Mosquitto")
# message(STATUS "--> will download and compile")
# if (MOSQUITTO_BUILT)
# 	include_directories(${MOSQUITTO_INCLUDE_DIR})
# 	list (APPEND UMUNDO_LIBRARIES ${MOSQUITTO_LIBRARY})
# 	list (APPEND UMUNDO_PREREQS libre)
# else()
# 	message(FATAL_ERROR "Could not build Mosquitto")
# endif()

############################################################
# Create config.h
############################################################

# determine path seperator to shorten filenames in Debug.cpp
if (WIN32)
    SET(PATH_SEPERATOR "\\\\")
else()
	SET(PATH_SEPERATOR "/")
endif()

include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strndup HAVE_STRNDUP)
if (NOT HAVE_STRNDUP)
	add_definitions("-DNO_STRNDUP")
endif()

CHECK_FUNCTION_EXISTS(strnlen HAVE_STRNLEN)
if (NOT HAVE_STRNLEN)
	add_definitions("-DNO_STRNLEN")
endif()

include(CheckIncludeFile)
CHECK_INCLUDE_FILE(stdbool.h HAVE_STDBOOL_H)

# enable config.h style compile time options and add as "umundo/config.h"
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/umundo/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

# have version.h in installations
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/umundo/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/umundo/version.h)

# gcc is picky when it comes to argument order
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	if (NOT ANDROID)
		add_definitions("-fPIC")
	endif()
endif()

############################################################
# Handle sub-components
############################################################

# some more required libraries
if(THREAD_PTHREAD AND NOT ANDROID)
	LIST(APPEND UMUNDO_LIBRARIES "pthread")
endif()

if (ANDROID)
	LIST(APPEND UMUNDO_LIBRARIES log)
endif()

if (UNIX AND NOT APPLE AND NOT ANDROID)
	LIST(APPEND UMUNDO_LIBRARIES rt)
endif()

if (WIN32)
	LIST(APPEND UMUNDO_LIBRARIES Ws2_32)
endif()

if (APPLE)
	find_library(FOUNDATION NAMES Foundation REQUIRED)
	LIST(APPEND UMUNDO_LIBRARIES ${FOUNDATION})
endif()

include_directories("src")
add_subdirectory("src/umundo")

# define target
add_library(umundo ${UMUNDO_FILES} ${UMUNDO_HEADER_FILES} ${CMAKE_BINARY_DIR}/src/umundo/config.h)
if (DIST_PREPARE)
  set_target_properties(umundo PROPERTIES SOVERSION ${UMUNDO_VERSION})
endif()
target_link_libraries(umundo ${UMUNDO_LIBRARIES})
add_dependencies(umundo ${UMUNDO_PREREQS})

INSTALL_HEADERS(HEADERS ${UMUNDO_HEADER_FILES} COMPONENT headers)
INSTALL_LIBRARY(TARGETS umundo COMPONENT libraryCore)
set_target_properties(umundo PROPERTIES FOLDER "Libraries")

GET_DIRECTORY_PROPERTY(CORE_INCLUDES DIRECTORY "src/umundo" INCLUDE_DIRECTORIES)
include_directories(${CORE_INCLUDES})

if (BUILD_UMUNDO_TOOLS)
	add_subdirectory(tools)
endif()

if (NOT CMAKE_CROSSCOMPILING)
	add_subdirectory(docs)
endif()

# we have to use CMAKE_TOOLCHAIN_FILE to avoid "not used" warning with existing binary dir
if (CMAKE_TOOLCHAIN_FILE)
endif()

############################################################
# process other directories
############################################################

if (BUILD_BINDINGS)
	add_subdirectory(src/bindings)
endif()


############################################################
# Some concluding remarks
############################################################

set(ALL_LIBRARIES "")
list(APPEND ALL_LIBRARIES ${UMUNDO_LIBRARIES})
list(SORT ALL_LIBRARIES)
list(REMOVE_DUPLICATES ALL_LIBRARIES)

message(STATUS "Linking against external: ")
foreach(LIBRARY ${ALL_LIBRARIES})
	if (LIBRARY MATCHES "debug")
	elseif (LIBRARY MATCHES "optimized")
	else()
		STRING(REGEX REPLACE "${CMAKE_BINARY_DIR}" "BUILD_DIR" LIBRARY ${LIBRARY})
		STRING(REGEX REPLACE "${CMAKE_SOURCE_DIR}" "SOURCE_DIR" LIBRARY ${LIBRARY})
		
		message(STATUS "  ${LIBRARY}")
	endif()
endforeach()

set(AVAILABLE_LANGUAGE_BINDINGS "")
if (NOT BUILD_BINDINGS)
	set(AVAILABLE_LANGUAGE_BINDINGS "BUILD_BINDINGS explicitly set to false")
elseif (NOT SWIG_EXECUTABLE)
	set(AVAILABLE_LANGUAGE_BINDINGS "SWIG_EXECUTABLE does not contain a path to a swig binary")
else()
	if (TARGET "java")
		set(AVAILABLE_LANGUAGE_BINDINGS "java ${AVAILABLE_LANGUAGE_BINDINGS}")
	endif()

	if (TARGET "csharp")
		set(AVAILABLE_LANGUAGE_BINDINGS "csharp ${AVAILABLE_LANGUAGE_BINDINGS}")
	endif()

	if (TARGET "mono")
		set(AVAILABLE_LANGUAGE_BINDINGS "mono ${AVAILABLE_LANGUAGE_BINDINGS}")
	endif()

	if (TARGET "_umundoNativePython")
		set(AVAILABLE_LANGUAGE_BINDINGS "python ${AVAILABLE_LANGUAGE_BINDINGS}")
	endif()

	if (TARGET "umundoNativePerl")
		set(AVAILABLE_LANGUAGE_BINDINGS "perl ${AVAILABLE_LANGUAGE_BINDINGS}")
	endif()

	if (TARGET "umundoNativePHP")
		set(AVAILABLE_LANGUAGE_BINDINGS "php ${AVAILABLE_LANGUAGE_BINDINGS}")
	endif()

endif()

message(STATUS "Targets:")
message(STATUS "  Building umundo.core ........... : ON")
message(STATUS "  Building umundo.core RTP ....... : ${NET_RTP}")
message(STATUS "  Building umundo tests .......... : ${BUILD_TESTS}")
message(STATUS "  Building umundo tools .......... : ${BUILD_UMUNDO_TOOLS}")
message(STATUS "  Available language bindings .... : ${AVAILABLE_LANGUAGE_BINDINGS}")
if (BUILD_SHARED_LIBS AND BUILD_BINDINGS)
	message(STATUS "")
	message(STATUS "    Warning: Building language bindings BUILD_SHARED_LIBS=ON")
	message(STATUS "             introduces runtime dependency to libumundo")
	message(STATUS "")
endif()

message(STATUS "General information:")
message(STATUS "  Build type ..................... : ${CMAKE_BUILD_TYPE}")
if (BUILD_SHARED_LIBS)
	message(STATUS "  Building library as ............ : SHARED")
else()
	message(STATUS "  Building library as ............ : STATIC")
endif()

if (BUILD_PREFER_STATIC_LIBRARIES)
	message(STATUS "  Preferring dependent libraries . : STATIC ")
else()
	message(STATUS "  Preferring dependent libraries . : SHARED")
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UC)
message(STATUS "  CXX Flags   : " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UC}})
message(STATUS "  C   Flags   : " ${CMAKE_C_FLAGS} " " ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_UC}})

get_directory_property( CDEFLIST DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
foreach( DEF ${CDEFLIST} )
    set( CDEFS "${CDEFS} ${DEF}" )
endforeach()

message(STATUS "  Definitions : " ${CDEFS})


STRING(REGEX REPLACE "${CMAKE_BINARY_DIR}" "BUILD_DIR" REL_CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
STRING(REGEX REPLACE "${CMAKE_SOURCE_DIR}" "SOURCE_DIR" REL_CMAKE_LIBRARY_OUTPUT_DIRECTORY ${REL_CMAKE_LIBRARY_OUTPUT_DIRECTORY})

STRING(REGEX REPLACE "${CMAKE_BINARY_DIR}" "BUILD_DIR" REL_CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
STRING(REGEX REPLACE "${CMAKE_SOURCE_DIR}" "SOURCE_DIR" REL_CMAKE_RUNTIME_OUTPUT_DIRECTORY ${REL_CMAKE_RUNTIME_OUTPUT_DIRECTORY})

message(STATUS "  Libraries will end up in ....... : " ${REL_CMAKE_LIBRARY_OUTPUT_DIRECTORY})
message(STATUS "  Binaries will end up in ........ : " ${REL_CMAKE_RUNTIME_OUTPUT_DIRECTORY})

############################################################
# Tests
############################################################

if(BUILD_TESTS)
	add_subdirectory(tests)
	include (CTest)
endif()

############################################################
# Installation / Provide package target
############################################################

# see contrib/cmake/CPackUMundo.cmake
include(contrib/cmake/CPackUMundo.cmake)

cmake_policy(POP)

# This must always be last!
include(CPack)
