if (CMAKE_CROSSCOMPILING)
	return()
endif()

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

SET(CMAKE_SWIG_FLAGS "-c++")
if (64BIT_HOST)
	list(APPEND CMAKE_SWIG_FLAGS "-D64BIT_HOST")
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
SWIG_LINK_LIBRARIES(umundoNativePython ${UMUNDOCORE_LIBRARIES})

set_target_properties(_umundoNativePython PROPERTIES FOLDER "Bindings")

INSTALL_LIBRARY(TARGETS _umundoNativePython COMPONENT librarySwig DESTINATION share/umundo/lib)
