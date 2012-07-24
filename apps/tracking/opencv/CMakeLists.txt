FIND_PACKAGE(OpenCV QUIET)
FIND_LIBRARY(ARUCO_LIBS
 	NAMES aruco
)

if (OpenCV_LIBS AND ARUCO_LIBS AND S11N_PROTOBUF)

	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	UMUNDO_PROTOBUF_GENERATE_CPP_S11N(PROTO_SRCS PROTO_HDRS "${PROJECT_SOURCE_DIR}/s11n/interfaces/protobuf/Pose.proto")

	add_executable(pose-publisher 
		main.cpp 
		ArucoPosePublisher.cpp 
		${PROTO_SRCS}
	)
	target_link_libraries(pose-publisher 
		${OpenCV_LIBS}
		${UMUNDOCORE_LIBRARIES}
		${UMUNDOS11N_LIBRARIES}
		${ARUCO_LIBS}
	)
	set_target_properties(pose-publisher PROPERTIES FOLDER "Apps")

else()
	message(STATUS "Not building ArucoPosePublisher - missing libraries:")
	message(STATUS "\tOpenCV: ${OpenCV_LIBS}")
	message(STATUS "\tAruco: ${ARUCO_LIBS}")
	message(STATUS "\tor not building with S11N_PROTOBUF: ${S11N_PROTOBUF}")
endif()