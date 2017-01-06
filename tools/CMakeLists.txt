# on 64Bit builds we have unresolved symbols
# message("linking umundo-monitor")
# message("UMUNDO_LIBRARIES: ${UMUNDO_LIBRARIES}")
# message("UMUNDOS11N_LIBRARIES: ${UMUNDOS11N_LIBRARIES}")

set(GETOPT_WIN32)
if (WIN32)
	set(GETOPT_WIN32 ${PROJECT_SOURCE_DIR}/contrib/src/xgetopt/XGetopt.cpp)	
endif()

if (BUILD_UMUNDO_S11N)
	add_executable(umundo-monitor umundo-monitor.cpp ${GETOPT_WIN32})

	target_link_libraries(umundo-monitor umundo)
	set_target_properties(umundo-monitor PROPERTIES FOLDER "Tools")

	INSTALL_EXECUTABLE(
		TARGETS umundo-monitor
		COMPONENT tools 
	)
endif()

if (NET_RTP)
	add_executable(umundo-rtp-pub umundo-rtp-pub.cpp)
	target_link_libraries(umundo-rtp-pub umundo)
	set_target_properties(umundo-rtp-pub PROPERTIES FOLDER "Tools")

	INSTALL_EXECUTABLE(
		TARGETS umundo-rtp-pub
		COMPONENT tools 
	)

	add_executable(umundo-rtp-sub umundo-rtp-sub.cpp)
	target_link_libraries(umundo-rtp-sub umundo)
	set_target_properties(umundo-rtp-sub PROPERTIES FOLDER "Tools")

	INSTALL_EXECUTABLE(
		TARGETS umundo-rtp-sub
		COMPONENT tools 
	)
	
	find_package(Portaudio)
	if(PORTAUDIO_FOUND)
		include_directories(${PORTAUDIO_INCLUDE_DIRS})
		
		add_executable(umundo-multicast-phone-pub umundo-multicast-phone-pub.cpp)
		target_link_libraries(umundo-multicast-phone-pub umundo ${PORTAUDIO_LIBRARIES})
		set_target_properties(umundo-multicast-phone-pub PROPERTIES FOLDER "Tools")

		INSTALL_EXECUTABLE(
			TARGETS umundo-multicast-phone-pub
			COMPONENT tools 
		)
		
		add_executable(umundo-multicast-phone-sub umundo-multicast-phone-sub.cpp)
		target_link_libraries(umundo-multicast-phone-sub umundo ${PORTAUDIO_LIBRARIES})
		set_target_properties(umundo-multicast-phone-sub PROPERTIES FOLDER "Tools")

		INSTALL_EXECUTABLE(
			TARGETS umundo-multicast-phone-sub
			COMPONENT tools 
		)
		
		add_executable(umundo-phone umundo-phone.cpp)
		target_link_libraries(umundo-phone umundo ${PORTAUDIO_LIBRARIES})
		set_target_properties(umundo-phone PROPERTIES FOLDER "Tools")

		INSTALL_EXECUTABLE(
			TARGETS umundo-phone
			COMPONENT tools 
		)
	endif()
	
	file(GLOB POTENTIAL_GLUT "C:/Program Files/freeglut*" "C:/Program Files (x86)/freeglut*")
	SET(GLUT_ROOT_PATH ${POTENTIAL_GLUT})
	
	find_package(Freenect)
	find_package(OpenGL)
	find_package(GLUT)
	if(FREENECT_FOUND)
		include_directories(${FREENECT_INCLUDE_DIR})
		
		add_executable(umundo-kinect-pub umundo-kinect-pub.cpp ${GETOPT_WIN32})
		target_link_libraries(umundo-kinect-pub umundo ${FREENECT_LIBRARY})
		set_target_properties(umundo-kinect-pub PROPERTIES FOLDER "Tools")

		INSTALL_EXECUTABLE(
			TARGETS umundo-kinect-pub
			COMPONENT tools 
		)
	endif()
	
	if(OPENGL_FOUND AND GLUT_FOUND)
		include_directories(${OPENGL_INCLUDE_DIR})
		include_directories(${GLUT_INCLUDE_DIR})
		
		add_executable(umundo-kinect-sub umundo-kinect-sub.cpp ${GETOPT_WIN32})
		target_link_libraries(umundo-kinect-sub umundo ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
		set_target_properties(umundo-kinect-sub PROPERTIES FOLDER "Tools")

		INSTALL_EXECUTABLE(
			TARGETS umundo-kinect-sub
			COMPONENT tools 
		)
	endif()

endif()

add_executable(umundo-pingpong umundo-pingpong.cpp  ${GETOPT_WIN32})
target_link_libraries(umundo-pingpong umundo)
set_target_properties(umundo-pingpong PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-pingpong
	COMPONENT tools 
)

add_executable(umundo-stream-send umundo-stream-send.cpp)
target_link_libraries(umundo-stream-send umundo)
set_target_properties(umundo-stream-send PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-stream-send
	COMPONENT tools 
)

add_executable(umundo-stream-receive umundo-stream-receive.cpp)
target_link_libraries(umundo-stream-receive umundo)
set_target_properties(umundo-stream-receive PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-stream-receive
	COMPONENT tools 
)

add_executable(umundo-debug umundo-debug.cpp ${GETOPT_WIN32})
target_link_libraries(umundo-debug umundo)
set_target_properties(umundo-debug PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-debug
	COMPONENT tools 
)


add_executable(umundo-capture umundo-capture.cpp ${GETOPT_WIN32})
target_link_libraries(umundo-capture umundo)
set_target_properties(umundo-capture PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-capture
	COMPONENT tools 
)


add_executable(umundo-replay umundo-replay.cpp ${GETOPT_WIN32})
target_link_libraries(umundo-replay umundo)
set_target_properties(umundo-replay PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-replay
	COMPONENT tools 
)


if (BUILD_UMUNDO_S11N AND BUILD_UMUNDO_RPC AND OFF)
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
	
		add_executable(umundo-echo-service umundo-echo-service.cpp ${PROTO_SVC_FILES})
		target_link_libraries(umundo-echo-service umundo umundoserial umundorpc)
		set_target_properties(umundo-echo-service PROPERTIES FOLDER "Tools")
		add_dependencies(umundo-echo-service protoc-umundo-cpp-rpc) 

		INSTALL_EXECUTABLE(
			TARGETS umundo-echo-service
			COMPONENT tools 
		)
	endif()
endif()

# add_executable(umundo-bridge umundo-bridge.cpp ${GETOPT_WIN32})
# target_link_libraries(umundo-bridge umundo)
# set_target_properties(umundo-bridge PROPERTIES FOLDER "Tools")

# INSTALL_EXECUTABLE(
# 	TARGETS umundo-bridge
# 	COMPONENT tools
# )

add_executable(umundo-throughput umundo-throughput.cpp ${GETOPT_WIN32} ${PROJECT_SOURCE_DIR}/contrib/src/lz4/datagen.c)
target_link_libraries(umundo-throughput umundo)
set_target_properties(umundo-throughput PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-throughput
	COMPONENT tools 
)
