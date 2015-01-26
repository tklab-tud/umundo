if (BUILD_UMUNDO_S11N AND BUILD_UMUNDO_RPC)
	# process .proto files
	if (PROTOBUF_PROTOC_EXECUTABLE)
		# include_directories(${CMAKE_BINARY_DIR}/protobuf/generated)

		file(GLOB_RECURSE PROTOBUF_INTERFACES ${PROJECT_SOURCE_DIR}/tests/rpc/services/protobuf/*.proto)
		include_directories(${CMAKE_BINARY_DIR})
		set(PROTO_SVC_FILES)

		UMUNDO_PROTOBUF_GENERATE_CPP_S11N(PROTO_SRCS PROTO_HDRS ${PROTOBUF_INTERFACES})
		# list(APPEND UMUNDORPC_HEADER_FILES ${PROTO_HDRS})
		list(APPEND PROTO_SVC_FILES ${PROTO_SRCS})

		UMUNDO_PROTOBUF_GENERATE_CPP_RPC(PROTO_SRCS PROTO_HDRS ${PROTOBUF_INTERFACES})
		# list(APPEND UMUNDORPC_FILES ${PROTO_HDRS})
		list(APPEND PROTO_SVC_FILES ${PROTO_SRCS})
		
		add_executable(test-rpc-client test-rpc-client.cpp ${PROTO_SVC_FILES})
		target_link_libraries(test-rpc-client ${UMUNDOCORE_LIBRARIES})
		target_link_libraries(test-rpc-client ${UMUNDORPC_LIBRARIES})
		target_link_libraries(test-rpc-client ${UMUNDOS11N_LIBRARIES})
		target_link_libraries(test-rpc-client ${UMUNDOUTIL_LIBRARIES})
		set_target_properties(test-rpc-client PROPERTIES FOLDER "Tests")
		add_dependencies(ALL_TESTS test-rpc-client)
 
		add_executable(test-rpc-server test-rpc-server.cpp ${PROTO_SVC_FILES})
		target_link_libraries(test-rpc-server ${UMUNDOCORE_LIBRARIES})
		target_link_libraries(test-rpc-server ${UMUNDORPC_LIBRARIES})
		target_link_libraries(test-rpc-server ${UMUNDOS11N_LIBRARIES})
		target_link_libraries(test-rpc-server ${UMUNDOUTIL_LIBRARIES})
		set_target_properties(test-rpc-server PROPERTIES FOLDER "Tests")
		add_dependencies(ALL_TESTS test-rpc-server)
	
		add_executable(test-rpc-api test-api.cpp ${PROTO_SVC_FILES})
		target_link_libraries(test-rpc-api ${UMUNDOCORE_LIBRARIES})
		target_link_libraries(test-rpc-api ${UMUNDORPC_LIBRARIES})
		target_link_libraries(test-rpc-api ${UMUNDOS11N_LIBRARIES})
		target_link_libraries(test-rpc-api ${UMUNDOUTIL_LIBRARIES})
		set_target_properties(test-rpc-api PROPERTIES FOLDER "Tests")
		# add_test(test-rpc-api ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-rpc-api)
		add_dependencies(ALL_TESTS test-rpc-api)
	
	endif() 
endif()

