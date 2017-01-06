include(ExternalProject)

if (BUILD_PREFER_STATIC_LIBRARIES)
	set(CMAKE_PARAM_SHARED "-DBUILD_SHARED_LIBS=OFF")
	if ("${CMAKE_GENERATOR}" STREQUAL "Xcode")
		set(LIBRARY_SUFFIX "a")
		set(LIBRARY_PREFIX "Debug/")
	elseif (UNIX)
		set(LIBRARY_SUFFIX "a")
	elseif(WIN32)
		set(LIBRARY_SUFFIX "lib")
	endif()
	set(LIBSQUASH_LIBRARY ${CMAKE_BINARY_DIR}/deps/libsquash/src/libsquash-build/squash/${LIBRARY_PREFIX}libsquash0.8.${LIBRARY_SUFFIX})
else()
	set(CMAKE_PARAM_SHARED "-DBUILD_SHARED_LIBS=ON")
	if ("${CMAKE_GENERATOR}" STREQUAL "Xcode")
		set(LIBRARY_SUFFIX "dylib")
		set(LIBRARY_PREFIX "Debug/")
	elseif (UNIX)
		set(LIBRARY_SUFFIX "so")
	elseif(WIN32)
		set(LIBRARY_SUFFIX "dll")
	endif()
	set(LIBSQUASH_LIBRARY ${CMAKE_BINARY_DIR}/deps/libsquash/src/libsquash-build/squash/${LIBRARY_PREFIX}libsquash0.8.${LIBRARY_SUFFIX})
endif()

if (NOT LIBRARY_SUFFIX)
	message(FATAL_ERROR "Unknown platform!")
endif()

set(LIBSQUASH_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/libsquash/src/libsquash)
list(APPEND LIBSQUASH_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/libsquash/src/libsquash-build)
set(LIBSQUASH_BUILT ON)

if (CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_PARAM_TOOLCHAIN "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()
if (ANDROID_ABI)
	set(CMAKE_PARAM_ANDROID_ABI "-DANDROID_ABI=${ANDROID_ABI}")
endif()

if (ANDROID_NATIVE_API_LEVEL)
	set(CMAKE_PARAM_API_LEVEL "-DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}")
endif()


# if (${CMAKE_GENERATOR} MATCHES "Xcode")
# 	set(CMAKE_GENERATOR "Unix Makefiles")
# endif()

externalproject_add(libsquash
	GIT_REPOSITORY https://github.com/quixdb/squash
	BUILD_IN_SOURCE 0
	PREFIX ${CMAKE_BINARY_DIR}/deps/libsquash
	UPDATE_COMMAND ""
	PATCH_COMMAND 
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/libsquash/squash/CMakeLists.txt" <SOURCE_DIR>/squash/CMakeLists.txt
	CONFIGURE_COMMAND 
		${CMAKE_COMMAND} 
		-G ${CMAKE_GENERATOR}
		-D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		${CMAKE_PARAM_TOOLCHAIN} 
		${CMAKE_PARAM_ANDROID_ABI}
		${CMAKE_PARAM_API_LEVEL}
		${CMAKE_PARAM_SHARED}
		-DCMAKE_VERBOSE_MAKEFILE=OFF
		<SOURCE_DIR>
	INSTALL_COMMAND ""
)
