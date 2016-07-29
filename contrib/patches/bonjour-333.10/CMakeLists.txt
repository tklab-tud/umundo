cmake_minimum_required(VERSION 2.8.6)

project(bonjour)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(POLICY CMP0042)
	cmake_policy(SET CMP0042 OLD)
endif()

SET(BONJOUR_SRC_FILES
	mDNSShared/dnssd_clientshim.c 
	mDNSPosix/mDNSPosix.c 
	mDNSCore/mDNS.c
	mDNSCore/DNSCommon.c
	mDNSShared/mDNSDebug.c
	mDNSShared/GenLinkedList.c
	mDNSCore/uDNS.c
	mDNSShared/PlatformCommon.c
	mDNSPosix/mDNSUNP.c
	mDNSCore/DNSDigest.c
)

if (ANDROID_ABI)
	add_definitions(-DTARGET_OS_ANDROID)
endif()

add_definitions("-DDISC_BONJOUR_EMBED")

if(UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fwrapv -fno-strict-aliasing")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPID_FILE=\"/var/run/mdnsd.pid\"")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd\"")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTARGET_OS_LINUX -DNOT_HAVE_SA_LEN -D_SS_MAXSIZE")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_LINUX -DUSES_NETLINK")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -Os")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeclaration-after-statement")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMDNS_DEBUGMSGS=0")
	
elseif(WIN32)
	add_definitions("-DWIN32")
	add_definitions("-D_WIN32_WINNT=0x0501")
	add_definitions("-DMDNS_DEBUGMSGS=0")
	add_definitions("-DTARGET_OS_WIN32")
	add_definitions("-DWIN32_LEAN_AND_MEAN")
	add_definitions("-DUSE_TCP_LOOPBACK")
	add_definitions("-DPLATFORM_NO_STRSEP")
	add_definitions("-DPLATFORM_NO_EPIPE")
	add_definitions("-DPLATFORM_NO_RLIMIT")
	add_definitions("-DPID_FILE=\"\"")
	add_definitions("-DUNICODE")
	add_definitions("-D_UNICODE")
	add_definitions("-D_CRT_SECURE_NO_DEPRECATE")
	add_definitions("-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1")
	# add_definitions("-D_LEGACY_NAT_TRAVERSAL_")
	
	if (CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_definitions("-D_USE_32BIT_TIME_T")
	endif()
	
	LIST(REMOVE_ITEM BONJOUR_SRC_FILES "mDNSPosix/mDNSPosix.c")
	LIST(REMOVE_ITEM BONJOUR_SRC_FILES "mDNSPosix/mDNSUNP.c")
	LIST(REMOVE_ITEM BONJOUR_SRC_FILES "mDNSShared/PlatformCommon.c")
	
	file(GLOB WIN32_FILES 
		mDNSWindows/mDNSWin32.c
		mDNSWindows/Poll.c
		mDNSWindows/Secret.c
		mDNSWindows/SystemService/Firewall.cpp
	)
	SET(BONJOUR_SRC_FILES ${BONJOUR_SRC_FILES} ${WIN32_FILES})
	
	include_directories(mDNSWindows/SystemService)
	
endif()

include_directories(mDNSCore)
include_directories(mDNSShared)

add_library(mDNSEmbedded ${BONJOUR_SRC_FILES})
