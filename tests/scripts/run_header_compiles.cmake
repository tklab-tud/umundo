# minimize SCXML document and run
get_filename_component(TEST_FILE_NAME ${TESTFILE} NAME)


if ("${TEST_FILE_NAME}" MATCHES ".*umundo-objc.*")
  set(LANGUAGE "-ObjC++")
  set(OBJC_INCLUDE_DIR "-I${PROJECT_SOURCE_DIR}/src/bindings/objc")
endif()

set(COMPILE_CMD_BIN
"-c"
${LANGUAGE}
${OBJC_INCLUDE_DIR}
"-I${CMAKE_BINARY_DIR}"
"-I${PROJECT_BINARY_DIR}"
"-I${PROJECT_SOURCE_DIR}/src"
"${TESTFILE}")

execute_process(COMMAND ${CXX_BIN} ${COMPILE_CMD_BIN} RESULT_VARIABLE CMD_RESULT)
if(CMD_RESULT)
	message(FATAL_ERROR "Error running ${COMPILE_CMD_BIN}")
endif()


