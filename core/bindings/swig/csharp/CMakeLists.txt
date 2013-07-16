include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if (MSVC)
	# MSVC does not include inttypes.h but SWIG needs it
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../msvc)
endif()

SET(CMAKE_SWIG_FLAGS "")
SET(UMUNDO_CSHARP_NAMESPACE "org.umundo.core")
SET(UMUNDO_CSHARP_DIR "org/umundo/core")

# if (HOST_64BIT)
# 	list(APPEND CMAKE_SWIG_FLAGS "-DHOST_64BIT")
# endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(APPEND CMAKE_SWIG_FLAGS "-DDEBUG")
endif()

# we need ; to produce a space with the package .. weird
SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES SWIG_FLAGS "-w401;-namespace;${UMUNDO_CSHARP_NAMESPACE}")
SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES CPLUSPLUS ON)
SET(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/${UMUNDO_CSHARP_DIR}")

SWIG_ADD_MODULE(umundoNativeCSharp csharp umundocore.i)
set_target_properties(umundoNativeCSharp PROPERTIES FOLDER "Bindings")

# if (HOST_64BIT)
# 	set_target_properties(umundoNativeCSharp PROPERTIES COMPILE_FLAGS "-DHOST_64BIT")
# endif()


# I am not sure why we need it twice, but we do at least on android
SWIG_LINK_LIBRARIES(umundoNativeCSharp umundocore)
SWIG_LINK_LIBRARIES(umundoNativeCSharp ${UMUNDOCORE_LIBRARIES})

INSTALL_LIBRARY(TARGETS umundoNativeCSharp COMPONENT librarySwig DESTINATION share/umundo/lib)
