# generate JNI library and create a jar
# Make from within Eclipse fails miserably with the whole thing
# if(CMAKE_GENERATOR MATCHES "Eclipse.*" OR CMAKE_ECLIPSE_VERSION GREATER 0)
# 	message(STATUS "NOTE: Eclipse fails with dependency on *.java")
# 	return()
# endif()

if (NOT CMAKE_CROSSCOMPILING)
	find_package(JNI)
	if(JNI_FOUND)
		include_directories(${JNI_INCLUDE_DIRS})
	else()
		message(STATUS "No JNI libraries found - not building Java wrappers")
		return()
	endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if (MSVC)
	# MSVC does not include inttypes.h but SWIG needs it
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../msvc)
endif()

SET(CMAKE_SWIG_FLAGS "")
SET(UMUNDO_JAVA_PACKAGE "org.umundo.core")
SET(UMUNDO_JAVA_DIR "org/umundo/core")

# we need ; to produce a space with the package .. weird
SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES SWIG_FLAGS "-package;${UMUNDO_JAVA_PACKAGE}")
SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES CPLUSPLUS ON)
SET(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/${UMUNDO_JAVA_DIR}")

SWIG_ADD_MODULE(umundoNativeJava java umundocore.i)
foreach(JNI_LIBRARY ${JNI_LIBRARIES})
	if (NOT ${JNI_LIBRARY} MATCHES ".*jawt.*")
		SWIG_LINK_LIBRARIES(umundoNativeJava ${JNI_LIBRARY})
	endif()
endforeach()
set_target_properties(umundoNativeJava PROPERTIES FOLDER "Bindings")

# I am not sure why we need it twice, but we do at least on android
SWIG_LINK_LIBRARIES(umundoNativeJava ${UMUNDOCORE_LIBRARIES})
SWIG_LINK_LIBRARIES(umundoNativeJava ${UMUNDOCORE_LIBRARIES})

INSTALL_LIBRARY(TARGETS umundoNativeJava COMPONENT librarySwig)
