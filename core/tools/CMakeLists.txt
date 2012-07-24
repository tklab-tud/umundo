if (WIN32)
	add_executable(umundo-monitor umundo-monitor.cpp ${PROJECT_SOURCE_DIR}/contrib/snippets/XGetopt.cpp)
else()
	add_executable(umundo-monitor umundo-monitor.cpp)
endif()

# on 64Bit builds we have unresolved symbols
# message("linking umundo-monitor")
# message("UMUNDOCORE_LIBRARIES: ${UMUNDOCORE_LIBRARIES}")
# message("UMUNDOS11N_LIBRARIES: ${UMUNDOS11N_LIBRARIES}")

target_link_libraries(umundo-monitor ${UMUNDOCORE_LIBRARIES} ${UMUNDOS11N_LIBRARIES})
set_target_properties(umundo-monitor PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-monitor
	COMPONENT tools 
)

add_executable(umundo-pingpong umundo-pingpong.cpp)
target_link_libraries(umundo-pingpong ${UMUNDOCORE_LIBRARIES})
set_target_properties(umundo-pingpong PROPERTIES FOLDER "Tools")

INSTALL_EXECUTABLE(
	TARGETS umundo-pingpong
	COMPONENT tools 
)
