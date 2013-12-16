# only reenable if we found an implementation
set(BUILD_UMUNDO_RPC OFF)
include(UseUMundo)

###########################################
# Protocol Buffers
###########################################

find_package(Protobuf)
if (PROTOBUF_FOUND AND RPC_PROTOBUF AND BUILD_UMUNDO_TOOLS)
	set(BUILD_UMUNDO_RPC ON)

# 	LIST(APPEND UMUNDORPC_LIBRARIES optimized ${PROTOBUF_LIBRARY})
# 	LIST(APPEND UMUNDORPC_LIBRARIES debug ${PROTOBUF_LIBRARY_DEBUG})
	include_directories(${PROTOBUF_INCLUDE_DIRS})

 	if (NOT CMAKE_CROSSCOMPILING)
		# we need the private protobuf headers for the generators
		include_directories(${PROJECT_SOURCE_DIR}/contrib/prebuilt/include)

		add_executable(protoc-umundo-cpp-rpc src/umundo/protoc-rpc/ServiceGeneratorCPP.cpp)
		target_link_libraries(protoc-umundo-cpp-rpc ${PROTOBUF_PROTOC_LIBRARIES} ${PROTOBUF_LIBRARIES})
		# this is weird - on debian we need to link against pthread for 'pthread_once'
		if (THREAD_PTHREAD)
			target_link_libraries(protoc-umundo-cpp-rpc pthread)
		endif()
		set_target_properties(protoc-umundo-cpp-rpc PROPERTIES FOLDER "Tools")

		install(
			TARGETS protoc-umundo-cpp-rpc
			DESTINATION bin
			PERMISSIONS WORLD_EXECUTE OWNER_EXECUTE GROUP_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
			COMPONENT tools
		)

		add_executable(protoc-umundo-java-rpc src/umundo/protoc-rpc/ServiceGeneratorJava.cpp)
		target_link_libraries(protoc-umundo-java-rpc ${PROTOBUF_PROTOC_LIBRARIES} ${PROTOBUF_LIBRARIES})
		if (THREAD_PTHREAD)
			target_link_libraries(protoc-umundo-java-rpc pthread)
		endif()
		set_target_properties(protoc-umundo-java-rpc PROPERTIES FOLDER "Tools")

		install(
			TARGETS protoc-umundo-java-rpc
			DESTINATION bin
			PERMISSIONS WORLD_EXECUTE OWNER_EXECUTE GROUP_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
			COMPONENT tools
		)

	endif()

	# process .proto files
	if (NOT PROTOBUF_PROTOC_EXECUTABLE)
		message(FATAL_ERROR "protoc binary required for serialization")
	endif()

	file(GLOB_RECURSE PROTOBUF_INTERFACES services/protobuf/*.proto)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	UMUNDO_PROTOBUF_GENERATE_CPP_S11N(PROTO_SRCS PROTO_HDRS ${PROTOBUF_INTERFACES})
	list(APPEND UMUNDORPC_HEADER_FILES ${PROTO_HDRS})
	list(APPEND UMUNDORPC_FILES ${PROTO_SRCS})

	UMUNDO_PROTOBUF_GENERATE_CPP_RPC(PROTO_SRCS PROTO_HDRS ${PROTOBUF_INTERFACES})
	list(APPEND UMUNDORPC_HEADER_FILES ${PROTO_HDRS})
	list(APPEND UMUNDORPC_FILES ${PROTO_SRCS})
endif()

LIST(APPEND UMUNDORPC_LIBRARIES umundorpc)

############################################################
# setup umundorpc library
############################################################


if (BUILD_UMUNDO_RPC)

	# include directories for our header files
	include_directories(src)

	# gather all header files
	file(GLOB_RECURSE UMUNDORPC_HEADER_FILES src/umundo/rpc/*.h src/umundo/rpc.h)

	LIST(APPEND UMUNDORPC_LIBRARIES umundocore)
	LIST(APPEND UMUNDORPC_LIBRARIES umundoserial)

	###########################################
	# gather implementation agnostic files
	###########################################

	file(GLOB RPC_FILES src/umundo/rpc/*.cpp)
	list(APPEND UMUNDORPC_FILES  ${RPC_FILES})

	add_library(umundorpc ${UMUNDORPC_FILES} ${UMUNDORPC_HEADER_FILES})
	if (DIST_PREPARE)
	  set_target_properties(umundorpc PROPERTIES SOVERSION ${UMUNDO_VERSION})
	endif()

	target_link_libraries(umundorpc ${UMUNDORPC_LIBRARIES})
	INSTALL_HEADERS(HEADERS ${UMUNDORPC_HEADER_FILES} COMPONENT headers)
	INSTALL_LIBRARY(TARGETS umundorpc COMPONENT libraryRPC)
	set_target_properties(umundorpc PROPERTIES FOLDER "Libraries")

	# ok to have in IDEs, but do not install
	LIST(APPEND UMUNDORPC_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/../umundo/config.h)

	LIST(SORT UMUNDORPC_HEADER_FILES)

endif()

############################################################
# Miscellaneous
############################################################

set(UMUNDORPC_FILES "${UMUNDORPC_FILES}" PARENT_SCOPE)
set(UMUNDORPC_LIBRARIES "${UMUNDORPC_LIBRARIES}" PARENT_SCOPE)
set(UMUNDORPC_HEADER_FILES "${UMUNDORPC_HEADER_FILES}" PARENT_SCOPE)
set(BUILD_UMUNDO_RPC "${BUILD_UMUNDO_RPC}" PARENT_SCOPE)
