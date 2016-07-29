include(ExternalProject)

if (BUILD_PREFER_STATIC_LIBRARIES)
	if (UNIX)
		set(LIBRARY_SUFFIX "a")
		set(LIBRARY_PREFIX "lib")
	elseif(WIN32)
		set(LIBRARY_SUFFIX "lib")
		set(LIBRARY_PREFIX "")
	endif()
else()
	set(CMAKE_PARAM_SHARED "-DBUILD_SHARED_LIBS=ON")
	if (APPLE)
		set(LIBRARY_SUFFIX "dylib")
		set(LIBRARY_PREFIX "lib")
	elseif (UNIX)
		set(LIBRARY_SUFFIX "so")
		set(LIBRARY_PREFIX "lib")
	elseif(WIN32)
		set(LIBRARY_SUFFIX "dll")
		set(LIBRARY_PREFIX "")
	endif()
endif()

if (NOT LIBRARY_SUFFIX)
	message(FATAL_ERROR "Unknown platform!")
endif()

set(LIBRE_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/libre/src/libre/include)
set(LIBRE_LIBRARY ${CMAKE_BINARY_DIR}/deps/libre/src/libre-build/${LIBRARY_PREFIX}re.${LIBRARY_SUFFIX})
set(LIBRE_BUILT ON)

if (CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_PARAM_TOOLCHAIN "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()
if (ANDROID_ABI)
	set(CMAKE_PARAM_ANDROID_ABI "-DANDROID_ABI=${ANDROID_ABI}")
endif()
if (ANDROID_NATIVE_API_LEVEL)
	set(CMAKE_PARAM_API_LEVEL "-DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}")
endif()

externalproject_add(libre
	URL http://creytiv.com/pub/re-0.4.17.tar.gz
	URL_MD5 5900337fd8c77515c87eae5bf0f8c091
	BUILD_IN_SOURCE 0
	PREFIX ${CMAKE_BINARY_DIR}/deps/libre
	PATCH_COMMAND 
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/libre/CMakeLists.txt" <SOURCE_DIR>/mk/CMakeLists.txt &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/libre/unistd.h" <SOURCE_DIR>/include/unistd.h
	CONFIGURE_COMMAND 
		${CMAKE_COMMAND} 
		-G ${CMAKE_GENERATOR}
		-D CMAKE_BUILD_TYPE=Release
		${CMAKE_PARAM_TOOLCHAIN} 
		${CMAKE_PARAM_ANDROID_ABI}
		${CMAKE_PARAM_API_LEVEL}
		${CMAKE_PARAM_SHARED}
		"<SOURCE_DIR>/mk/"
	INSTALL_COMMAND ""
)


