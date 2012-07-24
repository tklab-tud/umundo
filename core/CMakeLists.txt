############################################################
# Gather libraries and header files
############################################################

# include directories for our header files
include_directories(src)
include_directories(bindings)

# gather all header files
file(GLOB_RECURSE UMUNDOCORE_HEADER_FILES src/umundo/*.h)

###########################################
# gather implementation agnostic files
###########################################
file(GLOB COMMON_FILES src/umundo/common/*.cpp)
file(GLOB CONN_FILES src/umundo/connection/*.cpp)
file(GLOB DISC_FILES src/umundo/discovery/*.cpp)
file(GLOB TREAD_FILES src/umundo/thread/*.cpp)

list(APPEND UMUNDOCORE_FILES 
	${COMMON_FILES}
	${CONN_FILES}
	${DISC_FILES}
	${TREAD_FILES}
)

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
# PCRE
###########################################

if (WIN32)
	set(PC_PCRE_LIBDIR "C:/Program Files/GnuWin32/lib")
	set(PC_PCRE_INCLUDEDIR "C:/Program Files/GnuWin32/include")
endif()

find_package(PCRE REQUIRED)
include_directories(${PCRE_INCLUDE_DIR})
LIST(APPEND UMUNDOCORE_LIBRARIES ${PCRE_LIBRARIES})


###########################################
# Bonjour
###########################################
if (DISC_BONJOUR OR DISC_BONJOUR_EMBED)
	file(GLOB_RECURSE DISC_BONJOUR_FILES src/umundo/discovery/bonjour/*.cpp)
	list(APPEND UMUNDOCORE_FILES ${DISC_BONJOUR_FILES})

	if (NOT APPLE AND NOT IOS)
		# bonjour library is automatically available on mac

		if (DISC_BONJOUR_EMBED)
			find_package(BonjourEmbedded REQUIRED)
			include_directories(${BonjourEmbedded_INCLUDE_DIR})
			LIST(APPEND UMUNDOCORE_LIBRARIES ${BonjourEmbedded_LIBRARY})

			file(GLOB_RECURSE DISC_BONJOUR_EMBED_FILES src/umundo/discovery/bonjour/mDNSEmbedded.c)
			list(APPEND UMUNDOCORE_FILES ${DISC_BONJOUR_EMBED_FILES})
			
			if (WIN32)
				LIST(APPEND UMUNDOCORE_LIBRARIES "Iphlpapi")
				LIST(APPEND UMUNDOCORE_LIBRARIES "Netapi32")
			endif()

		else()
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

	file(GLOB_RECURSE DISC_AVAHI_FILES src/umundo/discovery/avahi/*.cpp)
	list(APPEND UMUNDOCORE_FILES ${DISC_AVAHI_FILES})
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

LIST(APPEND UMUNDOCORE_LIBRARIES umundocore)

############################################################
# process other directories
############################################################

add_subdirectory(bindings)    

############################################################
# setup umundocore library
############################################################

LIST(SORT UMUNDOCORE_HEADER_FILES)

add_library(umundocore ${UMUNDOCORE_FILES})
target_link_libraries(umundocore ${UMUNDOCORE_LIBRARIES})
INSTALL_HEADERS(HEADERS ${UMUNDOCORE_HEADER_FILES} COMPONENT headers)
INSTALL_LIBRARY(TARGETS umundocore COMPONENT libraryCore)
set_target_properties(umundocore PROPERTIES FOLDER "Libraries")

# ok to have in IDEs, but do not install
LIST(APPEND UMUNDOCORE_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/../umundo/config.h)


############################################################
# Miscellaneous
############################################################

set(UMUNDOCORE_FILES "${UMUNDOCORE_FILES}" PARENT_SCOPE)
set(UMUNDOCORE_LIBRARIES "${UMUNDOCORE_LIBRARIES}" PARENT_SCOPE)
set(UMUNDOCORE_HEADER_FILES "${UMUNDOCORE_HEADER_FILES}" PARENT_SCOPE)

