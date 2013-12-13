############################################################
# Gather libraries and header files
############################################################

# gather all header files
file(GLOB_RECURSE UMUNDOCORE_HEADER_FILES src/umundo/*.h)

# include directories for our header files
include_directories(src)
include_directories(bindings)

# add objective-c bindings on mac and ios
if (APPLE OR IOS)
	file(GLOB_RECURSE UMUNDOCORE_OBJC_FILES bindings/umundo-objc/core/*.mm)
	file(GLOB_RECURSE UMUNDOCORE_OBJC_HEADER_FILES bindings/umundo-objc/core/*.h)
	list(APPEND UMUNDOCORE_HEADER_FILES ${UMUNDOCORE_OBJC_HEADER_FILES})
	list(APPEND UMUNDOCORE_FILES ${UMUNDOCORE_OBJC_FILES})
	if (IOSSIM)
		foreach(UMUNDOCORE_OBJC_FILE ${UMUNDOCORE_OBJC_FILES})
#
# set_source_files_properties is local to the directory -> we cannot move this to bindings/:
# http://www.cmake.org/pipermail/cmake/2010-March/036012.html
#
			set_source_files_properties(${UMUNDOCORE_OBJC_FILE} PROPERTIES COMPILE_FLAGS "-fobjc-abi-version=2 -fobjc-legacy-dispatch")
		endforeach()
	endif()
	include_directories(.)
endif()

###########################################
# gather implementation agnostic files
###########################################
file(GLOB COMMON_FILES src/umundo/common/*.cpp)
file(GLOB CONN_FILES src/umundo/connection/*.cpp)
file(GLOB DISC_FILES src/umundo/discovery/Discovery*.cpp)
file(GLOB TREAD_FILES src/umundo/thread/*.cpp)

list(APPEND UMUNDOCORE_FILES
	${COMMON_FILES}
	${CONN_FILES}
	${DISC_FILES}
	${TREAD_FILES}
)

if (ANDROID) # android gcc complains about boost 1.51
  SET_SOURCE_FILES_PROPERTIES(src/umundo/common/UUID.cpp PROPERTIES COMPILE_FLAGS -Wno-uninitialized)
endif()

###########################################
# ZeroMQ
###########################################
if(NET_ZEROMQ)
	find_package(ZeroMQ REQUIRED)
	LIST(APPEND UMUNDOCORE_LIBRARIES ${ZeroMQ_LIBRARY})
	include_directories(${ZeroMQ_INCLUDE_DIR})

	file(GLOB_RECURSE NET_ZEROMQ_FILES src/umundo/connection/zeromq/*.cpp)
	list(APPEND UMUNDOCORE_FILES ${NET_ZEROMQ_FILES})
endif()

###########################################
# RTP
###########################################

if(NET_RTP)
	file(GLOB_RECURSE NET_RTP_FILES src/umundo/connection/rtp/*.cpp)
	list(APPEND UMUNDOCORE_FILES ${NET_RTP_FILES})
endif()

###########################################
# PCRE
###########################################

find_package(PCRE REQUIRED)
include_directories(${PCRE_INCLUDE_DIR})
LIST(APPEND UMUNDOCORE_LIBRARIES ${PCRE_LIBRARIES})

###########################################
# Bonjour
###########################################
if (DISC_BONJOUR OR DISC_AVAHI)
  file(GLOB DISC_MDNS_FILES src/umundo/discovery/MDNS*.cpp)
	list(APPEND UMUNDOCORE_FILES ${DISC_MDNS_FILES})
endif()

if (DISC_BONJOUR OR DISC_BONJOUR_EMBED)

	file(GLOB_RECURSE DISC_BONJOUR_FILES src/umundo/discovery/mdns/bonjour/*.cpp)
	list(APPEND UMUNDOCORE_FILES ${DISC_BONJOUR_FILES})

	if (DISC_BONJOUR_EMBED)
		find_package(BonjourEmbedded REQUIRED)
		include_directories(${BonjourEmbedded_INCLUDE_DIR})
		LIST(APPEND UMUNDOCORE_LIBRARIES ${BonjourEmbedded_LIBRARY})

		file(GLOB_RECURSE DISC_BONJOUR_EMBED_FILES src/umundo/discovery/mdns/bonjour/mDNSEmbedded.c)
		list(APPEND UMUNDOCORE_FILES ${DISC_BONJOUR_EMBED_FILES})

		if (WIN32)
			LIST(APPEND UMUNDOCORE_LIBRARIES "Iphlpapi")
			LIST(APPEND UMUNDOCORE_LIBRARIES "Netapi32")
		endif()

	else()
		if (NOT APPLE AND NOT IOS)
			# bonjour library is automatically available on mac

			find_package(Bonjour REQUIRED)
			include_directories(${Bonjour_INCLUDE_DIR})
			LIST(APPEND UMUNDOCORE_LIBRARIES ${Bonjour_LIBRARY})
		endif()
	endif()
endif()

###########################################
# Avahi
###########################################
if(DISC_AVAHI)
	find_package(Avahi REQUIRED)
	LIST(APPEND UMUNDOCORE_LIBRARIES ${Avahi_LIBRARIES})
	include_directories(${Avahi_INCLUDE_DIR})

	file(GLOB_RECURSE DISC_AVAHI_FILES src/umundo/discovery/mdns/avahi/*.cpp)
	list(APPEND UMUNDOCORE_FILES ${DISC_AVAHI_FILES})
endif()

###########################################
# Broadcast
###########################################
if(DISC_BROADCAST)
	file(GLOB_RECURSE DISC_BROADCAST_FILES src/umundo/discovery/Broadcast*.cpp)
	list(APPEND UMUNDOCORE_FILES ${DISC_BROADCAST_FILES})
endif()

###########################################
# Threads
###########################################
if(THREAD_PTHREAD AND NOT ANDROID)
	LIST(APPEND UMUNDOCORE_LIBRARIES "pthread")
endif()


###########################################
# additional libraries required for core
###########################################

if (ANDROID)
	LIST(APPEND UMUNDOCORE_LIBRARIES log)
endif()

if (UNIX AND NOT APPLE AND NOT ANDROID)
	LIST(APPEND UMUNDOCORE_LIBRARIES rt)
	LIST(APPEND UMUNDOCORE_LIBRARIES ${UMUNDOCORE_LIBRARIES})
endif()

if (WIN32)
	LIST(APPEND UMUNDOCORE_LIBRARIES Ws2_32)
endif()

if (APPLE)
	find_library(FOUNDATION NAMES Foundation REQUIRED)
	LIST(APPEND UMUNDOCORE_LIBRARIES ${FOUNDATION})
endif()

############################################################
# process other directories
############################################################

if (BUILD_BINDINGS)
	add_subdirectory(bindings)
endif()

############################################################
# setup umundocore library
############################################################

LIST(SORT UMUNDOCORE_HEADER_FILES)

add_library(umundocore ${UMUNDOCORE_FILES} ${UMUNDOCORE_HEADER_FILES} ${CMAKE_CURRENT_BINARY_DIR}/../umundo/config.h)
if (DIST_PREPARE)
  set_target_properties(umundocore PROPERTIES SOVERSION ${UMUNDO_VERSION})
endif()
target_link_libraries(umundocore ${UMUNDOCORE_LIBRARIES})
INSTALL_HEADERS(HEADERS ${UMUNDOCORE_HEADER_FILES} COMPONENT headers)
INSTALL_LIBRARY(TARGETS umundocore COMPONENT libraryCore)
set_target_properties(umundocore PROPERTIES FOLDER "Libraries")

# if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
# 	set_target_properties(umundocore PROPERTIES COMPILE_FLAGS "-Weffc++")
# endif()

# ok to have in IDEs, but do not install
LIST(APPEND UMUNDOCORE_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/../umundo/config.h)


############################################################
# Miscellaneous
############################################################

set(UMUNDOCORE_FILES "${UMUNDOCORE_FILES}" PARENT_SCOPE)
set(UMUNDOCORE_LIBRARIES "${UMUNDOCORE_LIBRARIES}" PARENT_SCOPE)
set(UMUNDOCORE_HEADER_FILES "${UMUNDOCORE_HEADER_FILES}" PARENT_SCOPE)

