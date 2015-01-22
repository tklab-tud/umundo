#
# To cross compile, choose a toolchain file (e.g. iOS):
# build$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../contrib/cmake/CrossCompile-iOS.cmake
#

#
# CMAKE_BUILD_TYPE:STRING              Supported are Debug and Release
# BUILD_PREFER_PREBUILT_LIBS:BOOL      Look for our dependencies in contrib/prebuilt first
# BUILD_PREFER_STATIC_LIBRARIES:BOOL   Prefer static libraries to resolve dependencies
# BUILD_PREFER_STATIC_PROTOBUF:BOOL    Prefer static protobuf libraries
# BUILD_PREFER_STATIC_PCRE:BOOL        Prefer static pcre libraries
# BUILD_PREFER_STATIC_ZEROMQ:BOOL      Prefer static zeromq libraries
# BUILD_PREFER_STATIC_RE:BOOL          Prefer static re libraries
# BUILD_SHARED_LIBS:BOOL               Whether to build shared or static libraries
# BUILD_TESTS:BOOL                     Build the tests
# BUILD_CONVENIENCE_LIB:BOOL           Build libumundo convenience library
# BUILD_BINDINGS:BOOL                  Build the language bindings as well
# BUILD_UMUNDO_S11N:BOOL               Build the C++ serialization layer
# BUILD_UMUNDO_RPC:BOOL                Build the C++ remote procedure call layer
# BUILD_UMUNDO_UTIL:BOOL               Build the C++ utility library
# BUILD_UMUNDO_APPS:BOOL               Build some example applications
# BUILD_UMUNDO_TOOLS:BOOL              Build the command-line tools
# BUILD_WITH_CPP11:BOOL                Enable C++11
# DIST_PREPARE:BOOL                    Prepare a distribution, everything ends up in SRC/package/PLATFORM
# CMAKE_CUSTOM_FIND_PATH:STRING        Append the given path when searching for dependencies
#

# we need 2.8.9 for java but travis only features 2.8.7
# TODO: give a sensible error message when building the jar fails
cmake_minimum_required(VERSION 2.8.7)

cmake_policy(PUSH)
IF((CMAKE_VERSION VERSION_EQUAL "2.8.12") OR (CMAKE_VERSION VERSION_GREATER "2.8.12"))
	CMAKE_POLICY(SET CMP0022 OLD) 
ENDIF() 

if(POLICY CMP0042)
	cmake_policy(SET CMP0042 OLD)
endif()
if(POLICY CMP0045)
	cmake_policy(SET CMP0045 OLD)
endif()
if(POLICY CMP0046)
	cmake_policy(SET CMP0046 OLD)
endif()
if(POLICY CMP0026)
	cmake_policy(SET CMP0026 OLD)
endif()
if(POLICY CMP0038)
	cmake_policy(SET CMP0038 OLD)
endif()

if(POLICY CMP0054)
	# Escape variables in if
	cmake_policy(SET CMP0054 OLD)
endif()


# specify umundo version
SET(UMUNDO_VERSION_MAJOR "0")
SET(UMUNDO_VERSION_MINOR "4")
SET(UMUNDO_VERSION_PATCH "5")
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
include("${CMAKE_MODULE_PATH}/FunctionExists.cmake")
include("${CMAKE_MODULE_PATH}/HeaderExists.cmake")

# CMake 2.8.11 reports AMD64 for Windows 64Bit, where earlier versions reported x86
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(CMAKE_SYSTEM_PROCESSOR "x86")
endif()
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(CMAKE_SYSTEM_PROCESSOR "i686")
endif()

# use folders in the IDEs for the various targets (e.g. Library, Testing, Tools ..)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# lower case version of system name and compiler for library paths
string(TOLOWER ${CMAKE_SYSTEM_NAME} CMAKE_SYSTEM_NAME_LC)
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} CMAKE_CXX_COMPILER_ID_LC)
if (WIN32)
	set(CMAKE_CXX_COMPILER_ID_LC ${CMAKE_CXX_COMPILER_ID_LC}-${MSVC_VERSION})
endif()

set(MSVC_NAME)
set(MSVC2010 OFF)
set(MSVC2013 OFF)
if (MSVC_VERSION LESS 1600)
	message(WARNING "Only MSVC2010 and MSVC2013 are supported via prebuilts")
elseif(MSVC_VERSION LESS 1800)
	set(MSVC2010 ON)
	set(MSVC_NAME msvc2010)
else()
	set(MSVC2013 ON)
	set(MSVC_NAME msvc2013)
endif()

################################################################
# Configuration of library type, linking and language bindings
################################################################

#
# There are a few considerations when building libraries to run on as 
# many platforms as possible while providing flexibility and convenience.
#
# 1. When you just hit cmake && make you get a set of dynamic libraries linked as dynamically as possible
#   1.1 Except on Windows where deploying libraries is a pain
#   1.2 Building shared libs on Windows fails as we expose STL in our public headers
#
# 2. When Building the language bindings, the resulting extensions should be as static as possible
#
# 3. There are convenience libraries name libumundo (as opposed to libumundo[core|serial|rpc])
#    which include / link against everything.
#
# Build static libraries and language bindings
# $ cmake -DDIST_PREPARE=ON -DBUILD_BINDINGS=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_PREFER_STATIC_LIBRARIES=ON
#
# Build libraries for distribution:
# $ cmake -DDIST_PREPARE=ON -DBUILD_BINDINGS=OFF
#


OPTION(BUILD_CONVENIENCE_LIB  "Build umundo convenience library" OFF)
OPTION(BUILD_TESTS            "Build umundo tests" OFF)
OPTION(BUILD_BINDINGS         "Build umundo language bindings" ON)
OPTION(DIST_PREPARE           "Put libraries into the lib folder of the source tree" OFF)
OPTION(BUILD_WITH_CPP11       "Build with C++11" OFF)
OPTION(ENABLE_TRACING         "Log functions as they are entered" OFF)
if (ENABLE_TRACING)
	add_definitions("-DENABLE_TRACING")
endif()

# setup defaults
if (CMAKE_CROSSCOMPILING AND ANDROID)
	OPTION(BUILD_UMUNDO_S11N "Build umundo serialization" OFF)
	OPTION(BUILD_UMUNDO_RPC  "Build umundo remote procedure calls" OFF)
	OPTION(BUILD_UMUNDO_UTIL "Build umundo utilities" OFF)
else()
	OPTION(BUILD_UMUNDO_S11N "Build umundo serialization" ON)
	OPTION(BUILD_UMUNDO_RPC  "Build umundo remote procedure calls" ON)
	OPTION(BUILD_UMUNDO_UTIL "Build umundo utilities" ON)
endif()
if (CMAKE_CROSSCOMPILING)
	OPTION(BUILD_UMUNDO_APPS "Build sample applications" OFF)
	OPTION(BUILD_UMUNDO_TOOLS "Build sample applications" OFF)
else()
	OPTION(BUILD_UMUNDO_APPS "Build sample applications" OFF)
	OPTION(BUILD_UMUNDO_TOOLS "Build sample applications" ON)
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
	set(FIND_LIBRARY_USE_LIB64_PATHS ON)
# 	LIST(APPEND CMAKE_PREFIX_PATH "/usr/local/lib64")
# 	LIST(APPEND CMAKE_PREFIX_PATH "/usr/lib64")
endif()

if (APPLE)
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
endif()

# is this a 64Bit host?
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(HOST_64BIT ON)
	set(64BIT_LIB_POSTFIX 64)
	# additional library paths to be searched
	set(FIND_LIBRARY_USE_LIB64_PATHS ON)
	if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
	  set(CMAKE_SYSTEM_PROCESSOR "x86_64")
	endif()
	
# 	LIST(APPEND CMAKE_PREFIX_PATH "/usr/local/lib64")
# 	LIST(APPEND CMAKE_PREFIX_PATH "/usr/lib64")
endif()

# We use the toolchain file from http://code.google.com/p/android-cmake/
if (CMAKE_CROSSCOMPILING AND ANDROID_ABI)
	set(ANDROID ON)
	set(CMAKE_CROSSCOMPILING_TARGET "android")
	set(CMAKE_SYSTEM_VERSION ${ANDROID_NATIVE_API_LEVEL})
endif()

#
# Setting the CMAKE_PREFIX_PATH to a list of directories will cause all CMake modules
# to look in these directories in addition to the system search paths:
# http://www.vtk.org/Wiki/CMake_Cross_Compiling#Searching_and_finding_external_software
#

SET(UMUNDO_PLATFORM_ID)
SET(UMUNDO_PLATFORM_PATH)

if (CMAKE_CROSSCOMPILING)	
	if (IOS)
		SET(UMUNDO_PLATFORM_ID "ios")
	elseif (ANDROID)
		SET(UMUNDO_PLATFORM_ID "android")
	else()
		SET(UMUNDO_PLATFORM_ID "${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}_64")
	endif()
else()
	if (WIN32 AND 64BIT_HOST)
		SET(UMUNDO_PLATFORM_ID "${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}_64")
	elseif (APPLE)
		SET(UMUNDO_PLATFORM_ID "${CMAKE_SYSTEM_NAME_LC}-i386")
	else()
		SET(UMUNDO_PLATFORM_ID "${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}")
	endif()
endif()


if (CMAKE_CROSSCOMPILING)
	if (IOS)
		SET(UMUNDO_PLATFORM_PATH "ios")
	elseif (ANDROID)
		SET(UMUNDO_PLATFORM_PATH "android/${ANDROID_ABI}")
	endif()
elseif (APPLE)
	SET(UMUNDO_PLATFORM_PATH "${UMUNDO_PLATFORM_ID}/${MACOSX_VERSION_MAJOR}.${MACOSX_VERSION_MINOR}/${CMAKE_CXX_COMPILER_ID_LC}")
else()
	set(UMUNDO_PLATFORM_PATH ${UMUNDO_PLATFORM_ID}/${CMAKE_CXX_COMPILER_ID_LC})
endif()
	

#message(FATAL_ERROR "UMUNDO_PLATFORM_ID: ${UMUNDO_PLATFORM_ID}")

SET(UMUNDO_PREBUILT_LIBRARY_PATH)
SET(UMUNDO_PREBUILT_HEADER_PATH "${PROJECT_SOURCE_DIR}/contrib/prebuilt")
SET(UMUNDO_PREBUILT_LIBRARY_PATH "${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_PLATFORM_PATH}")

# search roots for libraries
if(CMAKE_CROSSCOMPILING)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "ONLY")
	if (IOS)
		SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "BOTH") # we need both to find jni - we don't?
		SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM "BOTH")
		SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "BOTH")
	elseif (ANDROID)
		SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "BOTH") # we need both to find jni - we don't?
		SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM "BOTH")
		SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "BOTH")
		LIST(APPEND CMAKE_PREFIX_PATH "/usr/local/bin") # this is needed to find swig
		LIST(APPEND CMAKE_PREFIX_PATH "/opt/local/bin") # this is needed to find protoc
	endif ()
endif()

#
# Download prebuilt libraries
#

SET(UMUNDO_LIBRARY_HOST_URL_PREFIX "http://umundo.tk.informatik.tu-darmstadt.de/prebuilt" CACHE STRING "The root path of an URL where to look for prebuilt libraries.")

if (CMAKE_CROSSCOMPILING)
	if (IOS)
		SET(UMUNDO_LIBRARY_ARCHIVE_NAME umundo-prebuilt-ios.tgz)
	elseif(ANDROID)
		SET(UMUNDO_LIBRARY_ARCHIVE_NAME umundo-prebuilt-android.tgz)
	else()
	endif()
else()
  SET(UMUNDO_LIBRARY_ARCHIVE_NAME umundo-prebuilt-${UMUNDO_PLATFORM_ID}.tgz)
endif()

# delete directory if unversioned or too old
if (EXISTS "${PROJECT_SOURCE_DIR}/contrib/prebuilt/include/VERSION.txt")
	file (STRINGS "${PROJECT_SOURCE_DIR}/contrib/prebuilt/include/VERSION.txt" PREBUILT_INCUDES_VERSION)
endif()
if (NOT "${UMUNDO_VERSION}" VERSION_EQUAL "${PREBUILT_INCUDES_VERSION}")
	message(STATUS "Prebuilt headers unversioned, too old or non-existant - downloading (again)")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${PROJECT_SOURCE_DIR}/contrib/prebuilt/include
	)
endif()

if (EXISTS "${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_PLATFORM_ID}/VERSION.txt")
	file (STRINGS "${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_PLATFORM_ID}/VERSION.txt" PREBUILT_LIBRARIES_VERSION)
endif()
if (NOT "${UMUNDO_VERSION}" VERSION_EQUAL "${PREBUILT_LIBRARIES_VERSION}")
	message(STATUS "Prebuilt libraries unversioned, too old or non-existant - downloading (again) / ${PREBUILT_LIBRARIES_VERSION} vs ${UMUNDO_VERSION}")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_PLATFORM_ID}
	)
endif()

# download include directory
if (NOT EXISTS ${PROJECT_SOURCE_DIR}/contrib/prebuilt/include)
	message(STATUS "Downloading prebuilt library headers: ${UMUNDO_LIBRARY_HOST_URL_PREFIX}/${UMUNDO_VERSION}/umundo-prebuilt-include.tgz")
	file(DOWNLOAD ${UMUNDO_LIBRARY_HOST_URL_PREFIX}/${UMUNDO_VERSION}/umundo-prebuilt-include.tgz
		${PROJECT_SOURCE_DIR}/contrib/prebuilt/umundo-prebuilt-include.tgz
		INACTIVITY_TIMEOUT 60 STATUS DOWNLOAD_STATUS  SHOW_PROGRESS)
	list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
	list(GET DOWNLOAD_STATUS 1 STATUS_STRING)
	if(STATUS_CODE EQUAL 0)
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E tar xzf ${PROJECT_SOURCE_DIR}/contrib/prebuilt/umundo-prebuilt-include.tgz
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib/prebuilt/
		)
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_SOURCE_DIR}/contrib/prebuilt/umundo-prebuilt-include.tgz
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib/prebuilt/
		)
	else()
		message("Downloading prebuilt library headers failed with ${STATUS_STRING} - maybe the host is down?")
	endif()
	file(WRITE ${PROJECT_SOURCE_DIR}/contrib/prebuilt/include/VERSION.txt "${UMUNDO_VERSION}")
endif()

# download prebuilt libraries
if (NOT EXISTS ${UMUNDO_PREBUILT_LIBRARY_PATH})
	message(STATUS "Downloading prebuilt libraries: ${UMUNDO_LIBRARY_HOST_URL_PREFIX}/${UMUNDO_VERSION}/${UMUNDO_LIBRARY_ARCHIVE_NAME}")
	file(DOWNLOAD ${UMUNDO_LIBRARY_HOST_URL_PREFIX}/${UMUNDO_VERSION}/${UMUNDO_LIBRARY_ARCHIVE_NAME}
		${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_LIBRARY_ARCHIVE_NAME}
		INACTIVITY_TIMEOUT 60 STATUS DOWNLOAD_STATUS  SHOW_PROGRESS)
	list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
	list(GET DOWNLOAD_STATUS 1 STATUS_STRING)
	if(STATUS_CODE EQUAL 0)
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E tar xzf ${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_LIBRARY_ARCHIVE_NAME}
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib/prebuilt/
		)
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_LIBRARY_ARCHIVE_NAME}
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib/prebuilt/
		)
	else()
		message("Downloading prebuilt libraries failed with ${STATUS_STRING} - maybe this platform is not supported?")
	endif()
	file(WRITE ${PROJECT_SOURCE_DIR}/contrib/prebuilt/${UMUNDO_PLATFORM_ID}/VERSION.txt "${UMUNDO_VERSION}")
endif()

#
# BUILD_PREFER_PREBUILT_LIBS:
# Do we want to search system paths or contrib/prebuilt first?
#
if (CMAKE_CROSSCOMPILING)
	# always prefer prebuilt libraries for cross-compiling
	OPTION(BUILD_PREFER_PREBUILT_LIBS "Search libraries in contrib/prebuilt first" ON)
	SET(CMAKE_PREFIX_PATH "${UMUNDO_PREBUILT_LIBRARY_PATH};${UMUNDO_PREBUILT_HEADER_PATH};${CMAKE_PREFIX_PATH}")
else()
	OPTION(BUILD_PREFER_PREBUILT_LIBS "Search libraries in contrib/prebuilt first" ON)
	if (BUILD_PREFER_PREBUILT_LIBS)
		# for everything, we start our search in contrib/prebuilt
		SET(CMAKE_PREFIX_PATH "${UMUNDO_PREBUILT_LIBRARY_PATH};${UMUNDO_PREBUILT_HEADER_PATH};${CMAKE_PREFIX_PATH}")
	else()
		# using this trick, we search paths from find_* before CMAKE_PREFIX_PATH as '/' is prepended first
		SET(CMAKE_PREFIX_PATH "/;${CMAKE_PREFIX_PATH};${UMUNDO_PREBUILT_LIBRARY_PATH};${UMUNDO_PREBUILT_HEADER_PATH}")
	endif()
endif()

# prepend some directories given at cmake invocation time
if (CMAKE_CUSTOM_FIND_PATH)
	SET(CMAKE_PREFIX_PATH "${CMAKE_CUSTOM_FIND_PATH};${CMAKE_PREFIX_PATH}")
endif()

#message(FATAL_ERROR "CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")

message(STATUS "Searching for prebuilt libraries in: ${CMAKE_PREFIX_PATH}")

if (WIN32)
	include_directories(${PROJECT_SOURCE_DIR}/contrib/snippets)
endif()

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
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

	# best practices from scott meyers
	#add_definitions(-Weffc++)

	if (NOT ANDROID)
		set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fPIC")
	endif()

	# all warnings
	add_definitions(-Wall)

	# both needed for boost 1.51
	add_definitions(-Wno-attributes)
	add_definitions(-Wno-uninitialized)

	# we cannot tread warning as error with the "narrowing problem"
	# add_definitions(-Werror)

	if (BUILD_WITH_CPP11)
		add_definitions(-std=c++11)
		add_definitions(-DWITH_CPP11)
	endif()

	if(GCC_VERSION VERSION_EQUAL 4.7 OR GCC_VERSION VERSION_GREATER 4.7)
		# when compiling as C++11, narrowing is a warning but older compilers complain about the option
		add_definitions(-Wno-narrowing)
	endif()

	if(GCC_VERSION VERSION_EQUAL 4.8 OR GCC_VERSION VERSION_GREATER 4.8)
		add_definitions(-Wno-unused-local-typedefs)
		
		# gcc 4.8 features
		if (CMAKE_BUILD_TYPE MATCHES ".*Deb.*")
			if (HOST_64BIT)
				add_definitions(-fno-omit-frame-pointer)
				# we need to link against samoething to avoid undef refs tsan*
#				add_definitions(-fsanitize=thread)
#				add_definitions(-fsanitize=address)
			endif()
			add_definitions(-O0)
		endif()
	endif()

	# swig will throw a warning with optimization otherwise
	add_definitions(-fno-strict-aliasing)
	add_definitions("-Wno-unused-value -Wno-unused-function -Wno-sign-compare")

	if (NOT CMAKE_BUILD_TYPE MATCHES ".*Deb.*") # when not building for debug
		# add_definitions("-s")
    	#set(CMAKE_CXX_FLAGS "-s")  ## Strip binary for everything but debug builds
    	# set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
		# add_definitions("-mpreferred-stack-boundary=4")
		# add_definitions("-fmerge-constants")
		# add_definitions("-fno-rtti -DBOOST_NO_TYPEID")
		# add_definitions("-fno-exceptions")
		# add_definitions("-fno-inline")
		# add_definitions("-ffunction-sections -fdata-sections")
	else()
		add_definitions(-rdynamic)
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_definitions("-DZMQ_STATIC")
	if (BUILD_PREFER_STATIC_PCRE)
		add_definitions("-DPCRE_STATIC")
	endif()
	add_definitions("-D_SCL_SECURE_NO_WARNINGS")
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	
	# be very clear about linking debug / non-debug C++ runtimes
	foreach(FLAGS CMAKE_EXE_LINKER_FLAGS_RELEASE CMAKE_SHARED_LINKER_FLAGS_RELEASE CMAKE_MODULE_LINKER_FLAGS_RELEASE)
		set(${FLAGS} "${${FLAGS}} ${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099 /LTCG /NODEFAULTLIB:MSVCRTD")
	endforeach()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_definitions("-fPIC")
	
	if (BUILD_WITH_CPP11)
		add_definitions(-std=c++11)
		add_definitions(-DWITH_CPP11)
	endif()
	
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-parentheses-equality")
else()
	message(FATAL_ERROR "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# see http://www.mail-archive.com/cmake@cmake.org/msg23240.html
if (APPLE)
#	add_definitions("-D_DARWIN_UNLIMITED_SELECT")
	set(CMAKE_OSX_ARCHITECTURES "x86_64")
  if (MACOSX_VERSION VERSION_LESS 10.9)
    # forces libstc++
  	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)
    set(LIBCPP libstdc++)
  else()
    # forces libc++
  	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
    set(LIBCPP libc++)
  endif()
	foreach(FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
			set(${FLAGS} "${${FLAGS}} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -stdlib=${LIBCPP}")
  endforeach()
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
	SET(LOGLEVEL_S11N "1" CACHE STRING "Log-level for serialization - higher is more verbose" FORCE)
	SET(LOGLEVEL_COMMON "1" CACHE STRING "Log-level for common - higher is more verbose" FORCE)
	SET(LOGLEVEL_DISC "1" CACHE STRING "Log-level for discovery - higher is more verbose" FORCE)
	SET(LOGLEVEL_NET "1" CACHE STRING "Log-level for networking - higher is more verbose" FORCE)
else()
	SET(LOGLEVEL_S11N "2" CACHE STRING "Log-level for serialization - higher is more verbose" FORCE)
	SET(LOGLEVEL_COMMON "2" CACHE STRING "Log-level for common - higher is more verbose" FORCE)
	SET(LOGLEVEL_DISC "2" CACHE STRING "Log-level for discovery - higher is more verbose" FORCE)
	SET(LOGLEVEL_NET "2" CACHE STRING "Log-level for networking - higher is more verbose" FORCE)
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

SET(CMAKE_LIBRARY_POSTFIX ${CMAKE_${BUILD_TYPE}_POSTFIX})

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
OPTION(S11N_PROTOBUF "Use protobuf object serialization" ON)
OPTION(RPC_PROTOBUF "Use protobuf remote procedure calls" ON)

if (NET_ZEROMQ)
	SET(NET_ZEROMQ_SND_HWM "300000" CACHE STRING "Maximum queue size for publishers")
	SET(NET_ZEROMQ_RCV_HWM "300000" CACHE STRING "Maximum queue size for subscribers")
endif()

############################################################
# Library location, type and linking
############################################################

if (BUILD_UMUNDO_S11N AND S11N_PROTOBUF AND WIN32)
	# take a guess at PROTOBUF_SRC_ROOT_FOLDER
	if(EXISTS "${PROJECT_BINARY_DIR}/../protobuf-2.4.1/")
		set(PROTOBUF_SRC_ROOT_FOLDER "${PROJECT_BINARY_DIR}/../protobuf-2.4.1")
	elseif(EXISTS "${PROJECT_BINARY_DIR}/../../protobuf-2.4.1/")
		set(PROTOBUF_SRC_ROOT_FOLDER "${PROJECT_BINARY_DIR}/../../protobuf-2.4.1")
	endif()
endif()

# built shared or static libraries?
if(BUILD_SHARED_LIBS)
	add_definitions("-DCOMPILING_DLL")
	# set(BUILD_CONVENIENCE_LIB OFF)
else()
	add_definitions("-DUMUNDO_STATIC")
	set(BUILD_CONVENIENCE_LIB ON)
endif()

# where should libraries end up?
if (DIST_PREPARE)
	if (CMAKE_CROSSCOMPILING)
		set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/package/cross-compiled/${UMUNDO_PLATFORM_PATH})
	else()
		set(OUTPUT_DIR ${PROJECT_SOURCE_DIR}/package/${UMUNDO_PLATFORM_PATH})
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
# Find all our dependant libraries here so we can control
# their link type in a central place
############################################################

if (BUILD_PREFER_STATIC_LIBRARIES)
	OPTION(BUILD_PREFER_STATIC_PROTOBUF "Prefer static Protobuf libraries when linking" ON)
	OPTION(BUILD_PREFER_STATIC_PCRE "Prefer static PCRE libraries when linking" ON)
	OPTION(BUILD_PREFER_STATIC_ZEROMQ "Prefer static ZeroMQ libraries when linking" ON)
	OPTION(BUILD_PREFER_STATIC_RE "Prefer static RE libraries when linking" ON)
else()
	OPTION(BUILD_PREFER_STATIC_PROTOBUF "Prefer static Protobuf libraries when linking" OFF)
	OPTION(BUILD_PREFER_STATIC_PCRE "Prefer static PCRE libraries when linking" OFF)
	OPTION(BUILD_PREFER_STATIC_ZEROMQ "Prefer static ZeroMQ libraries when linking" OFF)
	OPTION(BUILD_PREFER_STATIC_RE "Prefer static RE libraries when linking" OFF)
endif()

if ((NOT APPLE) AND UNIX AND 64BIT_HOST) # pcre on debian is compiled without -fPIC
	set(BUILD_PREFER_STATIC_PCRE OFF)
endif()

# Boost
FIND_PATH(Boost_INCLUDE_DIR boost/version.hpp PATHS /usr/include)
include_directories(${Boost_INCLUDE_DIR})

# Protobuf
if (BUILD_PREFER_STATIC_PROTOBUF)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_STATIC})
else()
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SHARED})
endif()
if (BUILD_UMUNDO_S11N)
	find_package(Protobuf)
	if (PROTOBUF_FOUND)
		file(STRINGS "${PROTOBUF_INCLUDE_DIR}/google/protobuf/stubs/common.h" PROTOBUF_VERSION_HPP_CONTENTS REGEX "#define GOOGLE_PROTOBUF_VERSION ([0-9]*)")
		STRING(REGEX MATCH "[0-9]+" PROTOBUF_VERSION ${PROTOBUF_VERSION_HPP_CONTENTS})
	endif()
endif()

set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})

# PCRE
if (BUILD_PREFER_STATIC_PCRE)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_STATIC})
else()
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SHARED})
endif()
find_package(PCRE REQUIRED)
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})

# 0MQ
if (BUILD_PREFER_STATIC_ZEROMQ)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_STATIC})
else()
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SHARED})
endif()
find_package(ZeroMQ REQUIRED)
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})

# RE
if (BUILD_PREFER_STATIC_RE)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_STATIC})
else()
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_SHARED})
endif()
if (NET_RTP)
	find_package(RE)
  if (NOT RE_FOUND)
    set(NET_RTP OFF)
	else()
		include_directories(${RE_INCLUDE_DIR})
  endif()
endif()
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})

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

# have version.h in installations
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/core/src/umundo/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/core/src/umundo/version.h)

# gcc is picky when it comes to argument order
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	if (NOT ANDROID)
		add_definitions("-fPIC")
	endif()
endif()

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
	add_subdirectory(apps/fileserver)
	add_subdirectory(apps/tracking/opencv)
endif()
if (BUILD_UMUNDO_TOOLS)
	add_subdirectory(apps/tools)
endif()

if (NOT CMAKE_CROSSCOMPILING)
	add_subdirectory(docs)
endif()

# we have to use CMAKE_TOOLCHAIN_FILE to avoid "not used" warning with existing binary dir
if (CMAKE_TOOLCHAIN_FILE)
endif()

############################################################
# Convenience libraries
############################################################


if(BUILD_CONVENIENCE_LIB)
	set(_UMUNDO_COMP_NAMES "CORE;SERIAL;RPC;UTIL")
	set(_UMUNDO_COMP_BUILD)
	foreach(_UMUNDO_COMP_NAME ${_UMUNDO_COMP_NAMES})
		string(TOLOWER ${_UMUNDO_COMP_NAME} _UMUNDO_COMP_NAME_LC)
		GET_TARGET_PROPERTY(UMUNDOCOMP_LOCATION umundo${_UMUNDO_COMP_NAME_LC} LOCATION) # VAR target property
		if (UMUNDOCOMP_LOCATION)
			list(APPEND _UMUNDO_COMP_BUILD "umundo${_UMUNDO_COMP_NAME_LC}")
		endif()
	endforeach()

	if(BUILD_SHARED_LIBS)
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp "// auto-generated dummy to link shared libraries together\n" )
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp "#include <string>\n" )
		add_library(umundo ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp)
		target_link_libraries(umundo ${_UMUNDO_COMP_BUILD})
		INSTALL_LIBRARY(TARGETS umundo COMPONENT libraryCore)
		set_target_properties(umundo PROPERTIES FOLDER "Libraries")
	else()	
		# Build one huge static library (not implemented for all platforms)
		set(UMUNDO_STATIC_LIBRARIES "")
		set(UMUNDO_STATIC_LIBRARIES_SUFFIX)
		if(WIN32)
			set(UMUNDO_STATIC_LIBRARIES_SUFFIX "lib")
		else()
			set(UMUNDO_STATIC_LIBRARIES_SUFFIX "a")
		endif()

		set(UMUNDO_STATIC_LIBRARIES_TMP "")
		set(UMUNDO_COMP_BUILT "")
		set(_UMUNDO_COMP_NAMES "CORE;SERIAL;RPC;UTIL")
		foreach(_UMUNDO_COMP_NAME ${_UMUNDO_COMP_NAMES})
			string(TOLOWER ${_UMUNDO_COMP_NAME} _UMUNDO_COMP_NAME_LC)
			GET_TARGET_PROPERTY(UMUNDOCOMP_LOCATION umundo${_UMUNDO_COMP_NAME_LC} LOCATION) # VAR target property
			#message("${_UMUNDO_COMP_NAME}: ${UMUNDOCOMP_LOCATION}")
	 		if (UMUNDOCOMP_LOCATION)
				if (_UMUNDO_COMP_NAME STREQUAL "SERIAL")
					set(_UMUNDO_COMP_NAME S11N)
				endif()
				list(APPEND UMUNDO_COMP_BUILT umundo${_UMUNDO_COMP_NAME_LC})
				list(APPEND UMUNDO_STATIC_LIBRARIES_TMP ${UMUNDO${_UMUNDO_COMP_NAME}_LIBRARIES})
				# cmake does not add the CMAKE_LIBRARY_POSTFIX to the location
				# see: http://www.itk.org/Bug/print_bug_page.php?bug_id=7868
				STRING(REGEX REPLACE 
					"\\.${UMUNDO_STATIC_LIBRARIES_SUFFIX}" 
					"${CMAKE_LIBRARY_POSTFIX}.${UMUNDO_STATIC_LIBRARIES_SUFFIX}" 
					UMUNDOCOMP_LOCATION ${UMUNDOCOMP_LOCATION})
				if (WIN32)
					STRING(REGEX REPLACE "\\$\\(Configuration\\)/" "" UMUNDOCOMP_LOCATION ${UMUNDOCOMP_LOCATION})
				endif()
				list(APPEND UMUNDO_STATIC_LIBRARIES_TMP ${UMUNDOCOMP_LOCATION})
			endif()
		endforeach()

		# filter out static libraries for the current build type
		set(SKIP_NEXT_LIB OFF)
		foreach(UMUNDO_STATIC_LIBRARY ${UMUNDO_STATIC_LIBRARIES_TMP})
			if (SKIP_NEXT_LIB)
				set(SKIP_NEXT_LIB OFF)
			else()
				if (UMUNDO_STATIC_LIBRARY STREQUAL "debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
					set(SKIP_NEXT_LIB ON)
				endif()
				if (UMUNDO_STATIC_LIBRARY STREQUAL "optimized" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
					set(SKIP_NEXT_LIB ON)
				endif()

				get_filename_component(SUFFIX ${UMUNDO_STATIC_LIBRARY} EXT)
				if (SUFFIX STREQUAL ".${UMUNDO_STATIC_LIBRARIES_SUFFIX}")
					list(APPEND UMUNDO_STATIC_LIBRARIES ${UMUNDO_STATIC_LIBRARY})
				endif()
			endif()
		endforeach()

		list(SORT UMUNDO_STATIC_LIBRARIES)
		list(REMOVE_DUPLICATES UMUNDO_STATIC_LIBRARIES)
		#	message("UMUNDO_STATIC_LIBRARIES: ${UMUNDO_STATIC_LIBRARIES}")
		# message("UMUNDO_COMP_BUILT: ${UMUNDO_COMP_BUILT}")

		if (IOS OR APPLE)
			set(LIPO_INPUT_FILES "")
			if (IOS)
		    set(LIPO_CMD xcrun -sdk iphoneos lipo)
		    set(LIBTOOL_CMD xcrun -sdk iphoneos libtool)
			else()
  	    set(LIPO_CMD "lipo")
  	    set(LIBTOOL_CMD "libtool")
			endif()
			foreach(ARCH ${CMAKE_OSX_ARCHITECTURES})
				# message("ARCH: ${ARCH}")
				# @TODO: we should only depend on targets that are actually being built
				ADD_CUSTOM_COMMAND(
					OUTPUT    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libumundo${CMAKE_LIBRARY_POSTFIX}.${ARCH}.a
					COMMAND   ${LIBTOOL_CMD}
					ARGS      -arch_only ${ARCH} -static -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libumundo${CMAKE_LIBRARY_POSTFIX}.${ARCH}.a ${UMUNDO_STATIC_LIBRARIES} 1>/dev/null
					DEPENDS   ${UMUNDO_COMP_BUILT}
					COMMENT   "Running libtool on static libraries for ${ARCH}"
				)
				list(APPEND LIPO_INPUT_FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libumundo${CMAKE_LIBRARY_POSTFIX}.${ARCH}.a)
			endforeach()
			ADD_CUSTOM_TARGET(umundo ALL
				DEPENDS ${LIPO_INPUT_FILES} umundocore
				COMMAND ${LIPO_CMD} -create -output ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libumundo${CMAKE_LIBRARY_POSTFIX}.a ${LIPO_INPUT_FILES} && rm ${LIPO_INPUT_FILES}
				COMMENT "Building libumundo${CMAKE_LIBRARY_POSTFIX}.a convenience library")
		elseif(CMAKE_CROSSCOMPILING)
			# do nothing
		elseif(WIN32)
			if (CMAKE_GENERATOR MATCHES "Visual Studio.*")
				message(STATUS "Not building convenience umundo.lib as we do not know the build type at configure time")
			else()
				ADD_CUSTOM_COMMAND(
					OUTPUT    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundo${CMAKE_LIBRARY_POSTFIX}.lib
					COMMAND   lib.exe
					ARGS      /OUT:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}\\umundo${CMAKE_LIBRARY_POSTFIX}.lib ${UMUNDO_STATIC_LIBRARIES}
					DEPENDS   ${UMUNDO_COMP_BUILT}
					COMMENT   "Running lib.exe on static libraries"
				)
				ADD_CUSTOM_TARGET(umundo ALL
					DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundo${CMAKE_LIBRARY_POSTFIX}.lib
					COMMENT "Building umundo${CMAKE_LIBRARY_POSTFIX}.lib convenience library")
			endif()
		elseif(UNIX)
			ADD_CUSTOM_COMMAND(
				OUTPUT    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libumundo${CMAKE_LIBRARY_POSTFIX}.a
				COMMAND   ar
				ARGS      qf ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libumundo${CMAKE_LIBRARY_POSTFIX}.a ${UMUNDO_STATIC_LIBRARIES}
				DEPENDS   ${UMUNDO_COMP_BUILT}
				COMMENT   "Running ar on static libraries"
			)
			ADD_CUSTOM_TARGET(umundo ALL
				DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libumundo${CMAKE_LIBRARY_POSTFIX}.a
				COMMENT "Building umundo${CMAKE_LIBRARY_POSTFIX}.a convenience library")
		endif()
	endif()
endif()

############################################################
# Some concluding remarks
############################################################

set(ALL_LIBRARIES "")
list(APPEND ALL_LIBRARIES ${UMUNDOCORE_LIBRARIES})
list(APPEND ALL_LIBRARIES ${UMUNDOS11N_LIBRARIES})
list(APPEND ALL_LIBRARIES ${UMUNDORPC_LIBRARIES})
list(APPEND ALL_LIBRARIES ${UMUNDOUTIL_LIBRARIES})
list(SORT ALL_LIBRARIES)
list(REMOVE_DUPLICATES ALL_LIBRARIES)

message(STATUS "Linking against external:")
foreach(LIBRARY ${ALL_LIBRARIES})
	if (LIBRARY MATCHES "debug")
	elseif (LIBRARY MATCHES "optimized")
	elseif (LIBRARY MATCHES "umundo.*")
	else()
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
message(STATUS "  Building umundo.s11n ........... : ${BUILD_UMUNDO_S11N}")
message(STATUS "  Building umundo.rpc ............ : ${BUILD_UMUNDO_RPC}")
message(STATUS "  Building umundo.util ........... : ${BUILD_UMUNDO_UTIL}")
message(STATUS "  Building umundo tests .......... : ${BUILD_TESTS}")
message(STATUS "  Building umundo applications ... : ${BUILD_UMUNDO_APPS}")
message(STATUS "  Building umundo tools .......... : ${BUILD_UMUNDO_TOOLS}")
message(STATUS "  Available language bindings .... : ${AVAILABLE_LANGUAGE_BINDINGS}")
if (BUILD_SHARED_LIBS AND BUILD_BINDINGS)
	message(STATUS "")
	message(STATUS "    Warning: Building language bindings BUILD_SHARED_LIBS=ON")
	message(STATUS "             introduces runtime dependency to libumundocore")
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

if (BUILD_PREFER_PREBUILT_LIBS)
	STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/" "" REL_UMUNDO_PREBUILT_LIBRARY_PATH ${UMUNDO_PREBUILT_LIBRARY_PATH})
	message(STATUS "  Preferring dependent libraries . : from ${REL_UMUNDO_PREBUILT_LIBRARY_PATH}")
else()
	message(STATUS "  Preferring dependent libraries . : installed on system")
endif()


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

# see contrib/cmake/CPackUMundo.cmake
include(contrib/cmake/CPackUMundo.cmake)

cmake_policy(POP)

# This must always be last!
include(CPack)
