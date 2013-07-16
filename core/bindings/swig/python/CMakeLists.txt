if (CMAKE_CROSSCOMPILING)
	return()
endif()

set(Python_ADDITIONAL_VERSIONS)
find_package(PythonLibs)
if(NOT PYTHONLIBS_FOUND)
	return()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PYTHON_INCLUDE_DIRS})
if (MSVC)
	# MSVC does not include inttypes.h but SWIG needs it
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../msvc)
endif()

# -includeall?
SET(CMAKE_SWIG_FLAGS "-threads" "-c++")
if (HOST_64BIT)
	list(APPEND CMAKE_SWIG_FLAGS "-DHOST_64BIT")
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(APPEND CMAKE_SWIG_FLAGS "-DDEBUG")
endif()

SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES SWIG_FLAGS "-w401,362;")
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/umundocorePYTHON_wrap.c PROPERTIES LANGUAGE CXX)
SWIG_ADD_MODULE(umundoNativePython python umundocore.i)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/umundocorePYTHON_wrap.c PROPERTIES COMPILE_FLAGS "-Wno-dynamic-class-memaccess")
endif()

SWIG_LINK_LIBRARIES(umundoNativePython ${PYTHON_LIBRARIES})
SWIG_LINK_LIBRARIES(umundoNativePython umundocore)

set_target_properties(_umundoNativePython PROPERTIES FOLDER "Bindings")

set(UMUNDO_PYTHON_WRAPPER "umundoNativePython${CMAKE_LIBRARY_POSTFIX}.py")
string(REGEX REPLACE "NativePython" "" UMUNDO_PYTHON_WRAPPER_DEST ${UMUNDO_PYTHON_WRAPPER})

ADD_CUSTOM_COMMAND(
	TARGET _umundoNativePython
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy 
		${CMAKE_CURRENT_BINARY_DIR}/${UMUNDO_PYTHON_WRAPPER}
		${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${UMUNDO_PYTHON_WRAPPER_DEST}
)

INSTALL_LIBRARY(TARGETS _umundoNativePython COMPONENT librarySwig DESTINATION share/umundo/lib)
