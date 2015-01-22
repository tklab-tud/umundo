# only reenable if we found an implementation
set(BUILD_UMUNDO_S11N OFF)
include(UseUMUNDO)

###########################################
# Protocol Buffers
###########################################

find_package(Protobuf)
if (S11N_PROTOBUF AND PROTOBUF_FOUND)
	set(BUILD_UMUNDO_S11N ON)

	if(NOT EXISTS "${PROJECT_SOURCE_DIR}/contrib/prebuilt/include/protobuf-${PROTOBUF_VERSION}")
		message(WARNING "Not building umundo S11N: Cannot find protobuf compiler headers in ${PROJECT_SOURCE_DIR}/contrib/prebuilt/include/protobuf-${PROTOBUF_VERSION}")
		set(BUILD_UMUNDO_S11N OFF PARENT_SCOPE)
		return()
	else()
		include_directories(${PROJECT_SOURCE_DIR}/contrib/prebuilt/include/protobuf-${PROTOBUF_VERSION})
	endif()

	# protobuf-lite is not enough as we use reflection and dynamic messages
# 	LIST(APPEND UMUNDOS11N_LIBRARIES optimized ${PROTOBUF_LITE_LIBRARY})
# 	LIST(APPEND UMUNDOS11N_LIBRARIES debug ${PROTOBUF_LITE_LIBRARY_DEBUG})

	LIST(APPEND UMUNDOS11N_LIBRARIES optimized ${PROTOBUF_LIBRARY})
	LIST(APPEND UMUNDOS11N_LIBRARIES debug ${PROTOBUF_LIBRARY_DEBUG})
	include_directories(${PROTOBUF_INCLUDE_DIRS})

	file(GLOB_RECURSE S11N_PROTOBUF_FILES src/umundo/s11n/protobuf/*.cpp)
	list(APPEND UMUNDOS11N_FILES  ${S11N_PROTOBUF_FILES})

	if (NOT PROTOBUF_PROTOC_EXECUTABLE)
		message(FATAL_ERROR "protoc binary required for serialization")
	endif()

	# process .proto files
	file(GLOB_RECURSE PROTOBUF_INTERFACES interfaces/protobuf/*.proto)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	UMUNDO_PROTOBUF_GENERATE_CPP_S11N(PROTO_SRCS PROTO_HDRS ${PROTOBUF_INTERFACES})
	# list(APPEND UMUNDOS11N_HEADER_FILES ${PROTO_HDRS})
	# list(APPEND UMUNDOS11N_FILES ${PROTO_SRCS})

endif()

if(BUILD_UMUNDO_S11N)

	# include directories for our header files
	include_directories(src)

	# gather all header files
	file(GLOB_RECURSE UMUNDOS11N_HEADER_FILES src/umundo/*.h)

	LIST(APPEND UMUNDOS11N_LIBRARIES umundocore)

	# umundocore already links against Foundation.framework
	# if (APPLE)
	# 	find_library(FOUNDATION NAMES Foundation REQUIRED)
	# 	LIST(APPEND UMUNDOS11N_LIBRARIES ${FOUNDATION})
	# endif()

	###########################################
	# gather implementation agnostic files
	###########################################

	file(GLOB S11N_FILES src/umundo/s11n/*.cpp)
	list(APPEND UMUNDOS11N_FILES  ${S11N_FILES})

	############################################################
	# bindings
	############################################################

	if (APPLE OR IOS)
		file(GLOB_RECURSE UMUNDOS11N_OBJC_FILES src/umundo-objc/s11n/*.mm)
		file(GLOB_RECURSE UMUNDOS11N_OBJC_HEADER_FILES src/umundo-objc/s11n/*.h)
		list(APPEND UMUNDOS11N_HEADER_FILES ${UMUNDOS11N_OBJC_HEADER_FILES})
		list(APPEND UMUNDOS11N_FILES ${UMUNDOS11N_OBJC_FILES})
		if (IOSSIM)
			set_source_files_properties(${UMUNDOS11N_OBJC_FILES} PROPERTIES COMPILE_FLAGS "-fobjc-abi-version=2 -fobjc-legacy-dispatch")
		endif()
		include_directories(${PROJECT_SOURCE_DIR}/core/bindings/)
	endif()

	#LIST(APPEND UMUNDOS11N_LIBRARIES umundoserial)

	############################################################
	# setup umundoserial library
	############################################################

	add_library(umundoserial ${UMUNDOS11N_FILES} ${UMUNDOS11N_HEADER_FILES})
	if (DIST_PREPARE)
	  set_target_properties(umundoserial PROPERTIES SOVERSION ${UMUNDO_VERSION})
	endif()
	target_link_libraries(umundoserial ${UMUNDOS11N_LIBRARIES})
	INSTALL_HEADERS(HEADERS ${UMUNDOS11N_HEADER_FILES} COMPONENT headers)
	INSTALL_LIBRARY(TARGETS umundoserial COMPONENT libraryS11N)
	set_target_properties(umundoserial PROPERTIES FOLDER "Libraries")

	# ok to have in IDEs, but do not install
	LIST(APPEND UMUNDOS11N_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/../umundo/config.h)

	LIST(SORT UMUNDOS11N_HEADER_FILES)
endif()

############################################################
# Miscellaneous
############################################################

set(UMUNDOS11N_FILES "${UMUNDOS11N_FILES}" PARENT_SCOPE)
set(UMUNDOS11N_LIBRARIES "${UMUNDOS11N_LIBRARIES}" PARENT_SCOPE)
set(UMUNDOS11N_HEADER_FILES "${UMUNDOS11N_HEADER_FILES}" PARENT_SCOPE)
set(BUILD_UMUNDO_S11N "${BUILD_UMUNDO_S11N}" PARENT_SCOPE)

