############################################################
# Gather libraries and header files
############################################################

# gather all header files
file(GLOB_RECURSE UMUNDO_HEADER_FILES *.h)

# add objective-c bindings on mac and ios
if (APPLE OR IOS)
	file(GLOB_RECURSE UMUNDO_OBJC_FILES ${PROJECT_SOURCE_DIR}/src/bindings/objc/*.mm)
	file(GLOB_RECURSE UMUNDO_OBJC_HEADER_FILES ${PROJECT_SOURCE_DIR}/src/bindings/objc/*.h)
	list(APPEND UMUNDO_HEADER_FILES ${UMUNDO_OBJC_HEADER_FILES})
	list(APPEND UMUNDO_FILES ${UMUNDO_OBJC_FILES})

	foreach(UMUNDO_OBJC_FILE ${UMUNDO_OBJC_FILES})
		set_source_files_properties(${UMUNDO_OBJC_FILE} PROPERTIES COMPILE_FLAGS "-ObjC++")
		if (IOSSIM)
#
# set_source_files_properties is local to the directory -> we cannot move this to bindings/:
# http://www.cmake.org/pipermail/cmake/2010-March/036012.html
#
			set_source_files_properties(${UMUNDO_OBJC_FILE} PROPERTIES COMPILE_FLAGS "-fobjc-abi-version=2 -fobjc-legacy-dispatch")
		endif()
	endforeach()
	include_directories("${PROJECT_SOURCE_DIR}/src/bindings/objc")
endif()

###########################################
# gather implementation agnostic files
###########################################
file(GLOB COMMON_FILES *.cpp)
file(GLOB CONN_FILES connection/*.cpp)
file(GLOB DISC_FILES discovery/Discovery*.cpp)
file(GLOB THREAD_FILES thread/*.cpp)
if (BUILD_WITH_CXX11)
	list(REMOVE_ITEM THREAD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/thread/tinythread.cpp")
endif()
file(GLOB_RECURSE UTIL_FILES util/*.cpp)

list(APPEND UMUNDO_FILES
	${COMMON_FILES}
	${CONN_FILES}
	${DISC_FILES}
	${THREAD_FILES}
	${UTIL_FILES}
)

# miniz minimal compression library
if (BUILD_WITH_COMPRESSION_MINIZ)
	list(APPEND UMUNDO_FILES ${PROJECT_SOURCE_DIR}/contrib/src/miniz/miniz.c)
endif()
if(BUILD_WITH_COMPRESSION_FASTLZ)
	list(APPEND UMUNDO_FILES ${PROJECT_SOURCE_DIR}/contrib/src/fastlz/fastlz.c)
endif()
if(BUILD_WITH_COMPRESSION_LZ4)
	list(APPEND UMUNDO_FILES ${PROJECT_SOURCE_DIR}/contrib/src/lz4/lz4.c)
	list(APPEND UMUNDO_FILES ${PROJECT_SOURCE_DIR}/contrib/src/lz4/lz4frame.c)
	list(APPEND UMUNDO_FILES ${PROJECT_SOURCE_DIR}/contrib/src/lz4/lz4hc.c)
	list(APPEND UMUNDO_FILES ${PROJECT_SOURCE_DIR}/contrib/src/lz4/xxhash.c)
endif()

if (ANDROID) # android gcc complains about boost 1.51
  SET_SOURCE_FILES_PROPERTIES(common/UUID.cpp PROPERTIES COMPILE_FLAGS -Wno-uninitialized)
endif()

###########################################
# ZeroMQ
###########################################
if(NET_ZEROMQ)
	file(GLOB_RECURSE NET_ZEROMQ_FILES connection/zeromq/*.cpp)
	list(APPEND UMUNDO_FILES ${NET_ZEROMQ_FILES})
endif()

###########################################
# RTP
###########################################

if(NET_RTP)
	file(GLOB_RECURSE NET_RTP_FILES connection/rtp/*.cpp)
	list(APPEND UMUNDO_FILES ${NET_RTP_FILES})
endif()

###########################################
# Bonjour
###########################################
if (DISC_BONJOUR OR DISC_AVAHI)
  file(GLOB DISC_MDNS_FILES discovery/MDNS*.cpp)
	list(APPEND UMUNDO_FILES ${DISC_MDNS_FILES})
endif()

if (DISC_BONJOUR)

	file(GLOB_RECURSE DISC_BONJOUR_FILES discovery/mdns/bonjour/*.cpp)
	list(APPEND UMUNDO_FILES ${DISC_BONJOUR_FILES})

	if (DISC_BONJOUR_EMBED)

		file(GLOB_RECURSE DISC_BONJOUR_EMBED_FILES discovery/mdns/bonjour/mDNSEmbedded.c)
		list(APPEND UMUNDO_FILES ${DISC_BONJOUR_EMBED_FILES})

		if (WIN32)
			LIST(APPEND UMUNDO_LIBRARIES "Iphlpapi")
			LIST(APPEND UMUNDO_LIBRARIES "Netapi32")
		endif()
	endif()
		
endif()

###########################################
# Avahi
###########################################
if(DISC_AVAHI)
	file(GLOB_RECURSE DISC_AVAHI_FILES discovery/mdns/avahi/*.cpp)
	list(APPEND UMUNDO_FILES ${DISC_AVAHI_FILES})
endif()

###########################################
# Broadcast
###########################################
if(DISC_BROADCAST)
	file(GLOB_RECURSE DISC_BROADCAST_FILES discovery/Broadcast*.cpp)
	list(APPEND UMUNDO_FILES ${DISC_BROADCAST_FILES})
endif()


############################################################
# setup umundo library
############################################################

LIST(SORT UMUNDO_HEADER_FILES)

# if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
# 	set_target_properties(umundo PROPERTIES COMPILE_FLAGS "-Weffc++")
# endif()

# ok to have in IDEs, but do not install
LIST(APPEND UMUNDO_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/../umundo/config.h)


############################################################
# Miscellaneous
############################################################

set(UMUNDO_FILES "${UMUNDO_FILES}" PARENT_SCOPE)
set(UMUNDO_LIBRARIES "${UMUNDO_LIBRARIES}" PARENT_SCOPE)
set(UMUNDO_HEADER_FILES "${UMUNDO_HEADER_FILES}" PARENT_SCOPE)

