include(ExternalProject)

if (UNIX)
	set(LIBRARY_SUFFIX "a")
	set(LIBRARY_PREFIX "lib")
elseif(WIN32)
	set(LIBRARY_SUFFIX "lib")
	set(LIBRARY_PREFIX "")
endif()

if (NOT LIBRARY_SUFFIX)
	message(FATAL_ERROR "Unknown platform!")
endif()

set(BONJOUR_LIBRARY ${CMAKE_BINARY_DIR}/deps/bonjour/src/bonjour-build/${LIBRARY_PREFIX}mDNSEmbedded.${LIBRARY_SUFFIX})
set(BONJOUR_INCLUDE_DIR "")
list(APPEND BONJOUR_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/bonjour/src/bonjour/mDNSShared)
list(APPEND BONJOUR_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/bonjour/src/bonjour/mDNSCore)
if (UNIX OR ANDROID)
	list(APPEND BONJOUR_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/bonjour/src/bonjour/mDNSPosix)
endif()
if (WIN32)
	list(APPEND BONJOUR_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/bonjour/src/bonjour/mDNSWindows)
endif()

set(BONJOUR_BUILT ON)

if (CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_PARAM_TOOLCHAIN "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()
if (ANDROID_ABI)
	set(CMAKE_PARAM_ANDROID_ABI "-DANDROID_ABI=${ANDROID_ABI}")
endif()
if (ANDROID_NATIVE_API_LEVEL)
	set(CMAKE_PARAM_API_LEVEL "-DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}")
endif()

# if (OFF)
# 	add_definitions("-DWIN32")
# 	add_definitions("-D_WIN32_WINNT=0x0501")
# 	add_definitions("-DMDNS_DEBUGMSGS=0")
# 	add_definitions("-DTARGET_OS_WIN32")
# 	add_definitions("-DWIN32_LEAN_AND_MEAN")
# 	add_definitions("-DUSE_TCP_LOOPBACK")
# 	add_definitions("-DPLATFORM_NO_STRSEP")
# 	add_definitions("-DPLATFORM_NO_EPIPE")
# 	add_definitions("-DPLATFORM_NO_RLIMIT")
# 	add_definitions("-DPID_FILE=\"\"")
# 	add_definitions("-DUNICODE")
# 	add_definitions("-D_UNICODE")
# 	add_definitions("-D_CRT_SECURE_NO_DEPRECATE")
# 	add_definitions("-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1")
# 	add_definitions("-D_USE_32BIT_TIME_T")
# 	add_definitions("-D_LEGACY_NAT_TRAVERSAL_")
# endif()

externalproject_add(bonjour
	URL http://opensource.apple.com/tarballs/mDNSResponder/mDNSResponder-333.10.tar.gz
	URL_MD5 4e7daae255ca23093d24cb6268765bf5
	BUILD_IN_SOURCE 0
	PATCH_COMMAND
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/CMakeLists.txt" <SOURCE_DIR>/CMakeLists.txt &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSCore/mDNSEmbeddedAPI.h" <SOURCE_DIR>/mDNSCore/mDNSEmbeddedAPI.h &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSPosix/mDNSPosix.c" <SOURCE_DIR>/mDNSPosix/mDNSPosix.c &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSPosix/mDNSPosix.h" <SOURCE_DIR>/mDNSPosix/mDNSPosix.h &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSShared/dns_sd.h" <SOURCE_DIR>/mDNSShared/dns_sd.h &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSShared/dnssd_clientshim.c" <SOURCE_DIR>/mDNSShared/dnssd_clientshim.c &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSShared/dnssd_ipc.h" <SOURCE_DIR>/mDNSShared/dnssd_ipc.h &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSShared/PlatformCommon.c" <SOURCE_DIR>/mDNSShared/PlatformCommon.c &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSShared/uds_daemon.h" <SOURCE_DIR>/mDNSShared/uds_daemon.h &&
		${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/contrib/patches/bonjour-333.10/mDNSWindows/mDNSWin32.c" <SOURCE_DIR>/mDNSWindows/mDNSWin32.c
	CONFIGURE_COMMAND 
		${CMAKE_COMMAND}
		-G ${CMAKE_GENERATOR}
		-D CMAKE_BUILD_TYPE=Release
		${CMAKE_PARAM_TOOLCHAIN} 
		${CMAKE_PARAM_ANDROID_ABI}
		${CMAKE_PARAM_API_LEVEL}
		<SOURCE_DIR>
	INSTALL_COMMAND ""
	PREFIX ${CMAKE_BINARY_DIR}/deps/bonjour
)


