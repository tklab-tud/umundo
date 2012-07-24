if (NOT WIN32)
	return()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if (MSVC)
	# MSVC does not include inttypes.h but SWIG needs it
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../msvc)
endif()

SET(CMAKE_SWIG_FLAGS "")
SET(UMUNDO_CSHARP_NAMESPACE "org.umundo.core")
SET(UMUNDO_CSHARP_DIR "org/umundo/core")

# we need ; to produce a space with the package .. weird
SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES SWIG_FLAGS "-namespace;${UMUNDO_CSHARP_NAMESPACE}")
SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES CPLUSPLUS ON)
SET(CMAKE_SWIG_OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/${UMUNDO_CSHARP_DIR}")

SWIG_ADD_MODULE(umundocoreCSharp csharp umundocore.i)
set_target_properties(umundocoreCSharp PROPERTIES FOLDER "Bindings")

# I am not sure why we need it twice, but we do at least on android
SWIG_LINK_LIBRARIES(umundocoreCSharp ${UMUNDOCORE_LIBRARIES})

INSTALL_LIBRARY(TARGETS umundocoreCSharp COMPONENT librarySwig)
