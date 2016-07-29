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
SET(UMUNDO_JAVA_PACKAGE "org.umundo")
SET(UMUNDO_JAVA_DIR "org/umundo")

# we need ; to produce a space with the package .. weird
SET_SOURCE_FILES_PROPERTIES(umundo.i PROPERTIES SWIG_FLAGS "-w401;-package;${UMUNDO_JAVA_PACKAGE}")
SET_SOURCE_FILES_PROPERTIES(umundo.i PROPERTIES CPLUSPLUS ON)
SET(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/${UMUNDO_JAVA_DIR}")

SWIG_ADD_MODULE(umundoNativeJava java umundo.i)
foreach(JNI_LIBRARY ${JNI_LIBRARIES})
	if (NOT ${JNI_LIBRARY} MATCHES ".*jawt.*")
		SWIG_LINK_LIBRARIES(umundoNativeJava ${JNI_LIBRARY})
	endif()
endforeach()
set_target_properties(umundoNativeJava PROPERTIES FOLDER "Bindings")

# http://stackoverflow.com/a/32399514/990120
# set_target_properties(umundoNativeJava PROPERTIES NO_SONAME OFF)
string(REPLACE "<CMAKE_SHARED_LIBRARY_SONAME_CXX_FLAG><TARGET_SONAME>" "" CMAKE_CXX_CREATE_SHARED_MODULE "${CMAKE_CXX_CREATE_SHARED_MODULE}")
# I am not sure why we need it twice, but we do at least on android
SWIG_LINK_LIBRARIES(umundoNativeJava umundo)

# SWIG_LINK_LIBRARIES(umundoNativeJava ${UMUNDO_LIBRARIES} ${UMUNDOUTIL_LIBRARIES})

INSTALL_LIBRARY(TARGETS umundoNativeJava COMPONENT librarySwig DESTINATION share/umundo/bindings/java)
