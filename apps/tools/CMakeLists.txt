# on 64Bit builds we have unresolved symbols
# message("linking umundo-monitor")
# message("UMUNDOCORE_LIBRARIES: ${UMUNDOCORE_LIBRARIES}")
# message("UMUNDOS11N_LIBRARIES: ${UMUNDOS11N_LIBRARIES}")

if (BUILD_UMUNDO_S11N)
	if (WIN32)
		add_executable(umundo-monitor umundo-monitor.cpp ${PROJECT_SOURCE_DIR}/contrib/snippets/XGetopt.cpp)
	else()
		add_executable(umundo-monitor umundo-monitor.cpp)
	endif()

	target_link_libraries(umundo-monitor umundocore umundoserial)
	set_target_properties(umundo-monitor PROPERTIES FOLDER "Tools")

	INSTALL_EXECUTABLE(
		TARGETS umundo-monitor
		COMPONENT tools 
	)
endif()

add_executable(umundo-rtppingpong umundo-rtppingpong.cpp)
target_link_libraries(umundo-rtppingpong umundocore)
set_target_properties(umundo-rtppingpong PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-rtppingpong
	COMPONENT tools 
)

add_executable(umundo-pingpong umundo-pingpong.cpp)
target_link_libraries(umundo-pingpong umundocore)
set_target_properties(umundo-pingpong PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-pingpong
	COMPONENT tools 
)

if (WIN32)
	add_executable(umundo-debug umundo-debug.cpp ${PROJECT_SOURCE_DIR}/contrib/snippets/XGetopt.cpp)
else()
	add_executable(umundo-debug umundo-debug.cpp)
endif()
target_link_libraries(umundo-debug umundocore)
set_target_properties(umundo-debug PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-debug
	COMPONENT tools 
)


if (WIN32)
	add_executable(umundo-capture umundo-capture.cpp ${PROJECT_SOURCE_DIR}/contrib/snippets/XGetopt.cpp)
else()
	add_executable(umundo-capture umundo-capture.cpp)
endif()
target_link_libraries(umundo-capture umundocore)
set_target_properties(umundo-capture PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-capture
	COMPONENT tools 
)


if (WIN32)
	add_executable(umundo-replay umundo-replay.cpp ${PROJECT_SOURCE_DIR}/contrib/snippets/XGetopt.cpp)
else()
	add_executable(umundo-replay umundo-replay.cpp)
endif()
target_link_libraries(umundo-replay umundocore)
set_target_properties(umundo-replay PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-replay
	COMPONENT tools 
)

if (BUILD_UMUNDO_S11N AND BUILD_UMUNDO_RPC)
	add_executable(umundo-echo-service umundo-echo-service.cpp)
	target_link_libraries(umundo-echo-service umundocore umundoserial umundorpc)
	set_target_properties(umundo-echo-service PROPERTIES FOLDER "Tools")

	INSTALL_EXECUTABLE(
		TARGETS umundo-echo-service
		COMPONENT tools 
	)
endif()

if (NOT WIN32)
	add_executable(umundo-bridge umundo-bridge.cpp)
	target_link_libraries(umundo-bridge umundocore)
	set_target_properties(umundo-bridge PROPERTIES FOLDER "Tools")

	INSTALL_EXECUTABLE(
		TARGETS umundo-bridge
		COMPONENT tools 
	)
endif()