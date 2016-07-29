include(ExternalProject)

externalproject_add(libsquash
	GIT_REPOSITORY https://github.com/quixdb/squash
	BUILD_IN_SOURCE 0
	PREFIX ${CMAKE_BINARY_DIR}/deps/libsquash
	CONFIGURE_COMMAND ${CMAKE_COMMAND} "<SOURCE_DIR>"
	INSTALL_COMMAND ""
)

set(LIBCURL_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/libcurl/include)

if (APPLE)
	set(LIBSQUASH_LIBRARY ${CMAKE_BINARY_DIR}/deps/libcurl/lib/libcurl.a)
elseif(UNIX)
	set(LIBSQUASH_LIBRARY ${CMAKE_BINARY_DIR}/deps/libcurl/lib/libcurl.a)
elseif(WIN32)
	set(LIBSQUASH_LIBRARY ${CMAKE_BINARY_DIR}/deps/libcurl/lib/libcurl_a.lib)
else()
	message(FATAL_ERROR "Unknown platform!")
endif()


set(LIBSQUASH_BUILT ON)

