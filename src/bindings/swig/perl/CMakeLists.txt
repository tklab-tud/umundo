if (CMAKE_CROSSCOMPILING)
	return()
endif()

find_package(PerlLibs)
if(NOT PERLLIBS_FOUND)
	return()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PERL_INCLUDE_PATH})
if (MSVC)
	# MSVC does not include inttypes.h but SWIG needs it
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../msvc)
endif()

SET(CMAKE_SWIG_FLAGS "-c++")
if (HOST_64BIT)
	list(APPEND CMAKE_SWIG_FLAGS "-DHOST_64BIT")
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(APPEND CMAKE_SWIG_FLAGS "-DDEBUG")
endif()

SET_SOURCE_FILES_PROPERTIES(umundo.i PROPERTIES SWIG_FLAGS "-w401,362;")
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/umundoPERL_wrap.c PROPERTIES LANGUAGE CXX)
SWIG_ADD_MODULE(umundoNativePerl perl umundo.i)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/umundoPERL_wrap.c 
		PROPERTIES 
		COMPILE_FLAGS "-Wno-unused-value -Wno-duplicate-decl-specifier")
endif()

SWIG_LINK_LIBRARIES(umundoNativePerl ${PERL_LIBRARY})
SWIG_LINK_LIBRARIES(umundoNativePerl umundo umundoutil)

set_target_properties(umundoNativePerl PROPERTIES FOLDER "Bindings")

set(UMUNDO_PERL_WRAPPER "umundoNativePerl${CMAKE_LIBRARY_POSTFIX}.pm")
set(UMUNDO_PERL_LIBRARY "libumundoNativePerl${CMAKE_LIBRARY_POSTFIX}.so")

string(REGEX REPLACE "NativePerl" "" UMUNDO_PERL_WRAPPER_DEST ${UMUNDO_PERL_WRAPPER})

# this might be needed on other platforms as well - perl DynaLoader is picky about the name 
if (APPLE)
	string(REGEX REPLACE ".so$" ".dylib" UMUNDO_PERL_LIBRARY_DEST ${UMUNDO_PERL_LIBRARY})
	string(REGEX REPLACE "^lib" "" UMUNDO_PERL_LIBRARY_DEST ${UMUNDO_PERL_LIBRARY_DEST})
endif()

ADD_CUSTOM_COMMAND(
	TARGET umundoNativePerl
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy 
		${CMAKE_CURRENT_BINARY_DIR}/${UMUNDO_PERL_WRAPPER}
		${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${UMUNDO_PERL_WRAPPER_DEST}
		COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${UMUNDO_PERL_LIBRARY}
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${UMUNDO_PERL_LIBRARY_DEST}
)

INSTALL_LIBRARY(TARGETS umundoNativePerl COMPONENT librarySwig DESTINATION share/umundo/lib)
