#
# To cross compile, choose a toolchain file (e.g. iOS):
# build$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../contrib/cmake/CrossCompile-iOS.cmake
#
cmake_minimum_required(VERSION 2.8.4)

# specify umundo version
SET(UMUNDO_VERSION_MAJOR "0")
SET(UMUNDO_VERSION_MINOR "0")
SET(UMUNDO_VERSION_PATCH "5")

# build type has to be set before the project definition
SET(BUILD_TYPE_HELP "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug, Release, RelWithDebInfo, MinSizeRel.")
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING ${BUILD_TYPE_HELP})
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING ${BUILD_TYPE_HELP})
ENDIF()

project(umundo)
# use folders in the IDEs for the various targets (e.g. Library, Testing, Tools ..)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# lower case version of system name and compiler for library paths
string(TOLOWER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME_LC)
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} CMAKE_CXX_COMPILER_ID_LC)

############################################################
# Search paths for cross compiling and prebuilds
############################################################

# this happens when we ssh into windows to build there, see also:
# http://publib.boulder.ibm.com/infocenter/wxdinfo/v6r1/index.jsp?topic=/com.ibm.websphere.ops.doc/info/odoe_task/tcimgr_sshwin.html
if (NOT CMAKE_SYSTEM_PROCESSOR)
	set(CMAKE_SYSTEM_PROCESSOR x86)
endif()

# is this a 64Bit host?
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(64BIT_HOST ON)
	set(64BIT_LIB_POSTFIX 64)
	# additional library paths to be searched
	LIST(APPEND CMAKE_FIND_ROOT_PATH "/usr/local/lib64")
	LIST(APPEND CMAKE_FIND_ROOT_PATH "/usr/lib64")
	if(CMAKE_COMPILER_IS_GNUCXX)
		add_definitions("-fPIC") # only required with 64Bit builds?
	endif()
endif()

#
# Setting the CMAKE_FIND_ROOT_PATH to a list of directories will cause all CMake modules
# to look in these directories in addition to the system search paths:
# http://www.vtk.org/Wiki/CMake_Cross_Compiling#Searching_and_finding_external_software
#

SET(UMUNDO_PREBUILT_LIBRARY_PATH)
# search roots for libraries
if(CMAKE_CROSSCOMPILING)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "ONLY")
	if (IOS)
		SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "ONLY")
		SET(UMUNDO_PREBUILT_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/contrib/prebuilt/ios/${CMAKE_SYSTEM_VERSION}")
	elseif (ANDROID)
		SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "ONLY") # we need both to find jni - we don't?
		SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM "BOTH")
		SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "BOTH")
		SET(UMUNDO_PREBUILT_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/contrib/prebuilt/android/${ANDROID_NDK_ABI_EXT}")
		LIST(APPEND CMAKE_FIND_ROOT_PATH "/usr/local/bin") # this is needed to find swig
		LIST(APPEND CMAKE_FIND_ROOT_PATH "/opt/local/bin") # this is needed to find protoc
	endif ()
else ()
	# SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM "BOTH")
	# SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "BOTH")
	# SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "BOTH")
	if (WIN32 AND 64BIT_HOST)
		SET(UMUNDO_PREBUILT_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/contrib/prebuilt/${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}_64/${CMAKE_CXX_COMPILER_ID_LC}")
	else()
		SET(UMUNDO_PREBUILT_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/contrib/prebuilt/${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_CXX_COMPILER_ID_LC}")
	endif()
endif()

LIST(APPEND UMUNDO_PREBUILT_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/contrib/prebuilt")

#
# BUILD_PREFER_PREBUILT_LIBS:
# Do we want to search system path or contrib/prebuilt first?
#
if (CMAKE_CROSSCOMPILING)
	# always prefer prebuilt libraries for cross-compiling
	SET(CMAKE_FIND_ROOT_PATH "${UMUNDO_PREBUILT_LIBRARY_PATH};${CMAKE_FIND_ROOT_PATH}")
else()
	OPTION(BUILD_PREFER_PREBUILT_LIBS "Search libraries in contrib/prebuilt first" ON)
	if (BUILD_PREFER_PREBUILT_LIBS)
		# for everything, we start our search in contrib/prebuilt
		SET(CMAKE_FIND_ROOT_PATH "${UMUNDO_PREBUILT_LIBRARY_PATH};${CMAKE_FIND_ROOT_PATH}")
	else()
		# using this trick, we search paths from find_* before CMAKE_FIND_ROOT_PATH as '/' is prepended first
		SET(CMAKE_FIND_ROOT_PATH "/;${CMAKE_FIND_ROOT_PATH};${UMUNDO_PREBUILT_LIBRARY_PATH}")
	endif()
endif()

#message("CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")

if (WIN32)
	include_directories(${PROJECT_SOURCE_DIR}/contrib/snippets)
endif()

############################################################
# General setup
############################################################

# enable testing and actual output with tests
if (CMAKE_CROSSCOMPILING)
	OPTION(BUILD_TESTS "Build umundo tests" OFF) 
else()
	OPTION(BUILD_TESTS "Build umundo tests" OFF) 
endif()

if (BUILD_TESTS)
	enable_testing()
	SET(ENV{CTEST_OUTPUT_ON_FAILURE} ON)
endif()

OPTION(DIST_PREPARE "Put libraries into the lib folder of the source tree" OFF)

# where to find the cmake modules we distribute
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/contrib/cmake)
include("${CMAKE_MODULE_PATH}/UMUNDOMacros.cmake")
include("${CMAKE_MODULE_PATH}/FunctionExists.cmake")

# some compiler flags
if(CMAKE_COMPILER_IS_GNUCXX)
	# best practices from scott meyers
	#	add_definitions(-Weffc++)
	
	# all warnings
	add_definitions(-Wall)

	# when compiling as C++11, narrowing is a warning but older compilers complain about the option
	# add_definitions(-Wno-narrowing)

	# we cannot tread warning as error with the "narrowing problem"
	# add_definitions(-Werror)

	# swig will throw a warning with optimization otherwise
	add_definitions(-fno-strict-aliasing)
	add_definitions("-Wno-unused-value -Wno-unused-function -Wno-sign-compare")

	if (NOT CMAKE_BUILD_TYPE MATCHES ".*Deb.*")
    set(CMAKE_CXX_FLAGS "-s")  ## Strip binary for everything but debug builds
    # set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
		# add_definitions("-mpreferred-stack-boundary=4")
		# add_definitions("-fmerge-constants")
		# add_definitions("-fno-rtti -DBOOST_NO_TYPEID")
		# add_definitions("-fno-exceptions")
		# add_definitions("-fno-inline")
		# add_definitions("-ffunction-sections -fdata-sections")
	endif()
elseif (MSVC)
	add_definitions("-DZMQ_STATIC")	
	add_definitions("-D_SCL_SECURE_NO_WARNINGS")
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	# SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB")
	# SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:MSVCRTD.lib")
endif()

# build type as macro to strip logging in release builds
string(TOUPPER BUILD_${CMAKE_BUILD_TYPE} BUILD_TYPE)
add_definitions("-D${BUILD_TYPE}")

if (CMAKE_CROSSCOMPILING AND ANDROID)
	OPTION(BUILD_UMUNDO_S11N "Build umundo serialization" OFF) 
	OPTION(BUILD_UMUNDO_RPC "Build umundo remote procedure calls" OFF) 
	OPTION(BUILD_UMUNDO_UTIL "Build umundo utilities" OFF) 
else()
	OPTION(BUILD_UMUNDO_S11N "Build umundo serialization" ON) 
	OPTION(BUILD_UMUNDO_RPC "Build umundo remote procedure calls" ON) 
	OPTION(BUILD_UMUNDO_UTIL "Build umundo utilities" ON) 
endif()
if (CMAKE_CROSSCOMPILING)
	OPTION(BUILD_UMUNDO_APPS "Build sample applications" OFF) 
else()
	OPTION(BUILD_UMUNDO_APPS "Build sample applications" OFF) 
endif()

OPTION(BUILD_SHARED_LIBS "Build shared libraries" OFF) 
if (CMAKE_CROSSCOMPILING OR DIST_PREPARE)
	OPTION(BUILD_PREFER_STATIC_LIBRARIES "Prefer static libraries when resolving dependencies" ON)
else()
	OPTION(BUILD_PREFER_STATIC_LIBRARIES "Prefer static libraries when resolving dependencies" OFF)
endif()
OPTION(DEBUG_THREADS "Print log messages on mutex and monitor access" OFF) 
if (DEBUG_THREADS)
	add_definitions("-DDEBUG_THREADS")
endif()

# Enable verbose Makefiles to debug the build process itself
set(CMAKE_VERBOSE_MAKEFILE OFF)

if (CMAKE_BUILD_TYPE MATCHES Release)
	SET(LOGLEVEL_S11N "1" CACHE STRING "Log-level for serialization - higher is more verbose" FORCE)
	SET(LOGLEVEL_COMMON "1" CACHE STRING "Log-level for common - higher is more verbose" FORCE)
	SET(LOGLEVEL_DISC "1" CACHE STRING "Log-level for discovery - higher is more verbose" FORCE)
	SET(LOGLEVEL_NET "1" CACHE STRING "Log-level for networking - higher is more verbose" FORCE)
else()
	SET(LOGLEVEL_S11N "4" CACHE STRING "Log-level for serialization - higher is more verbose" FORCE)
	SET(LOGLEVEL_COMMON "4" CACHE STRING "Log-level for common - higher is more verbose" FORCE)
	SET(LOGLEVEL_DISC "4" CACHE STRING "Log-level for discovery - higher is more verbose" FORCE)
	SET(LOGLEVEL_NET "4" CACHE STRING "Log-level for networking - higher is more verbose" FORCE)
endif()

SET_PROPERTY(CACHE LOGLEVEL_COMMON PROPERTY STRINGS 0 1 2 3)
SET_PROPERTY(CACHE LOGLEVEL_DISC PROPERTY STRINGS 0 1 2 3)
SET_PROPERTY(CACHE LOGLEVEL_NET PROPERTY STRINGS 0 1 2 3)
SET_PROPERTY(CACHE LOGLEVEL_S11N PROPERTY STRINGS 0 1 2 3)

############################################################
# postfixes for all built targets depending on build type
############################################################

SET(CMAKE_DEBUG_POSTFIX "${64BIT_LIB_POSTFIX}_d")
SET(CMAKE_RELEASE_POSTFIX "${64BIT_LIB_POSTFIX}")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "${64BIT_LIB_POSTFIX}_rd")
SET(CMAKE_MINSIZEREL_POSTFIX "${64BIT_LIB_POSTFIX}_s")

############################################################
# Select implementations depending on build target
############################################################

# select implementations for discovery and network connectivity
if(UNIX AND NOT APPLE AND NOT IOS AND NOT ANDROID)
	# build with avahi on all unices but macosx
	OPTION(DISC_BONJOUR "Use bonjour for discovery" OFF)
	OPTION(DISC_BONJOUR_EMBED "Embed mDNS discovery service" OFF) 
	OPTION(DISC_AVAHI "Use avahi for discovery" ON) 
else()
	# build with bonjour everywhere else
	OPTION(DISC_BONJOUR "Use bonjour for discovery" ON) 
	OPTION(DISC_AVAHI "Use avahi for discovery" OFF) 	
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

# CMake does not allow explicit dependencies
if (DISC_BONJOUR_EMBED AND NOT DISC_BONJOUR)
	message(FATAL_ERROR "Cannot use embedded bonjour without bonjour - enable DISC_BONJOUR!")
endif()
if (DISC_BONJOUR AND DISC_AVAHI)
	message(FATAL_ERROR "Cannot use both avahi and bonjour for discovery - choose either DISC_BONJOUR or DISC_AVAHI!")
endif()
if (NOT DISC_AVAHI AND NOT DISC_BONJOUR)
	message(FATAL_ERROR "No discovery implementation choosen - enable either DISC_BONJOUR or DISC_AVAHI!")
endif()
if (BUILD_PREFER_PREBUILT_LIBS AND NOT BUILD_PREFER_STATIC_LIBRARIES)
	# this is ok as long as we do not ship dynamic libraries in contrib/prebuilt
#	message(STATUS "Potentially problematic dynamic linking into contrib/prebuilt!")
endif()

if(WIN32)
	SET(THREAD_WIN32 ON)
else()
	SET(THREAD_PTHREAD ON)
endif()

OPTION(NET_ZEROMQ "Use 0MQ for networking" ON)
OPTION(S11N_PROTOBUF "Use protobuf object serialization" ON) 
OPTION(RPC_PROTOBUF "Use protobuf remote procedure calls" ON) 

if (S11N_PROTOBUF AND WIN32)
	# take a guess at PROTOBUF_SRC_ROOT_FOLDER
	if(EXISTS "${PROJECT_BINARY_DIR}/../protobuf-2.4.1/")
		set(PROTOBUF_SRC_ROOT_FOLDER "${PROJECT_BINARY_DIR}/../protobuf-2.4.1")
	elseif(EXISTS "${PROJECT_BINARY_DIR}/../../protobuf-2.4.1/")
		set(PROTOBUF_SRC_ROOT_FOLDER "${PROJECT_BINARY_DIR}/../../protobuf-2.4.1")
	endif()
endif()

if (NET_ZEROMQ)
	SET(NET_ZEROMQ_SND_HWM "10000" CACHE STRING "Maximum queue size for publishers")
	SET(NET_ZEROMQ_RCV_HWM "10000" CACHE STRING "Maximum queue size for subscribers")
endif()

############################################################
# Library location, type and linking
############################################################

# built shared or static libraries?
if(BUILD_SHARED_LIBS)
	add_definitions("-DCOMPILING_DLL")
else()
	add_definitions("-DUMUNDO_STATIC")
endif()

# prefer static libraries for linking
if(BUILD_PREFER_STATIC_LIBRARIES)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

# where should libraries end up?
if (DIST_PREPARE)
	if (CMAKE_CROSSCOMPILING)
		string(TOLOWER ${CMAKE_CROSSCOMPILING_TARGET} CMAKE_CROSSCOMPILING_TARGET_LC)
		set(OUTPUT_DIR 
			${PROJECT_SOURCE_DIR}/package/cross-compiled/${CMAKE_CROSSCOMPILING_TARGET_LC}-${CMAKE_SYSTEM_VERSION}/${CMAKE_SYSTEM_PROCESSOR})
	else()
		set(OUTPUT_DIR 
			${PROJECT_SOURCE_DIR}/package/${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_CXX_COMPILER_ID_LC})
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

############################################################
# Dependancies used in more than one of our components
# are included here to avoid multiple find* statements
############################################################

FIND_PATH(Boost_INCLUDE_DIR boost/version.hpp PATHS /usr/include)
include_directories(${Boost_INCLUDE_DIR})


############################################################
# Create config.h
############################################################

# determine path seperator to shorten filenames in Debug.cpp
if (WIN32)
    SET(PATH_SEPERATOR "\\\\")
else()
	SET(PATH_SEPERATOR "/")
endif()

# enable config.h style compile time options and add as "umundo/config.h"
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/umundo/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

############################################################
# Handle sub-components
############################################################

# include_directories is not global so we grab our subdirs property
# and propagate them through the build tree
add_subdirectory(core)
GET_DIRECTORY_PROPERTY(CORE_INCLUDES DIRECTORY core INCLUDE_DIRECTORIES)
include_directories(${CORE_INCLUDES})

if (BUILD_UMUNDO_S11N)
	include_directories("s11n/src")
endif()
if (BUILD_UMUNDO_RPC)
	include_directories("rpc/src")
endif()
if (BUILD_UMUNDO_UTIL)
	include_directories("util/src")
endif()

if (BUILD_UMUNDO_S11N)
	add_subdirectory(s11n)
	GET_DIRECTORY_PROPERTY(S11N_INCLUDES DIRECTORY s11n INCLUDE_DIRECTORIES)
	include_directories(${S11N_INCLUDES})
endif()
if (BUILD_UMUNDO_RPC)
	add_subdirectory(rpc)
	GET_DIRECTORY_PROPERTY(RPC_INCLUDES DIRECTORY rpc INCLUDE_DIRECTORIES)
	include_directories(${RPC_INCLUDES})
endif()
if (BUILD_UMUNDO_UTIL)
	add_subdirectory(util)
	GET_DIRECTORY_PROPERTY(UTIL_INCLUDES DIRECTORY util INCLUDE_DIRECTORIES)
	include_directories(${UTIL_INCLUDES})
endif()
if (BUILD_UMUNDO_APPS)
	add_subdirectory(apps)
endif()

if (NOT CMAKE_CROSSCOMPILING)
	add_subdirectory(docs)
endif()

# we have to use CMAKE_TOOLCHAIN_FILE to avoid "not used" warning with existing binary dir
if (CMAKE_TOOLCHAIN_FILE)
endif()

############################################################
# Some concluding remarks
############################################################

message(STATUS "Targets:")
message(STATUS "  Building umundo.core")

if (BUILD_UMUNDO_S11N)
	message(STATUS "  Building umundo.s11n")
else()
	message(STATUS "  Not building umundo.s11n")
endif()

if (BUILD_UMUNDO_RPC)
	message(STATUS "  Building umundo.rpc")
else()
	message(STATUS "  Not building umundo.rpc")
endif()

if (BUILD_UMUNDO_UTIL)
	message(STATUS "  Building umundo.util")
else()
	message(STATUS "  Not building umundo.util")
endif()

if (BUILD_TESTS)
	message(STATUS "  Building umundo tests")
else()
	message(STATUS "  Not building umundo tests")
endif()

if (BUILD_UMUNDO_APPS)
	message(STATUS "  Building umundo applications")
else()
	message(STATUS "  Not building umundo applications")
endif()

message(STATUS "General information:")
message(STATUS "  Build type ${CMAKE_BUILD_TYPE}")
if (BUILD_SHARED_LIBS)
	message(STATUS "  Building as shared libraries")
else()
	message(STATUS "  Building as static libraries")
endif()

if (BUILD_PREFER_STATIC_LIBRARIES)
	message(STATUS "  Preferred static libraries when available")
else()
	message(STATUS "  Preferred shared libraries when available")
endif()

if (BUILD_PREFER_PREBUILT_LIBS)
	message(STATUS "  Preferred prebuilt libraries in contrib/prebuilt")
else()
	message(STATUS "  Preferred system installed libraries")
endif()

message(STATUS "Language bindings:")
GET_TARGET_PROPERTY(UMUNDOCOREJAVA_LOCATION umundocoreJava LOCATION)
if (UMUNDOCOREJAVA_LOCATION)
	message(STATUS "  Building Java bindings")	
else()
	message(STATUS "  Not building Java bindings")	
endif()

GET_TARGET_PROPERTY(UMUNDOCORECSHARP_LOCATION umundocoreCSharp LOCATION)
if (UMUNDOCORECSHARP_LOCATION)
	message(STATUS "  Building CSharp bindings")	
else()
	message(STATUS "  Not building CSharp bindings")	
endif()

message(STATUS "Libraries will end up in: " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

############################################################
# Tests and reports
############################################################

if (NOT CMAKE_CROSSCOMPILING)
	# tools moved down here as some depend on s11n (move to apps?)
	add_subdirectory(core/tools)
endif()

if(BUILD_TESTS)
	add_subdirectory(core/test)
	if (BUILD_UMUNDO_S11N)
		add_subdirectory(s11n/test)
	endif()
	if (BUILD_UMUNDO_RPC)
		add_subdirectory(rpc/test)
	endif()
	include (CTest)
endif()

############################################################
# Installation / Provide package target
############################################################

# see contrib/cmake/CPackUMundoPlatform.cmake
include(contrib/cmake/CPackUMundoPlatform.cmake)

# This must always be last!
include(CPack)
