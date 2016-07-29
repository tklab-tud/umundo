include(ExternalProject)

externalproject_add(libmosquitto
	URL http://mosquitto.org/files/source/mosquitto-1.4.9.tar.gz
	URL_MD5 67943e2c5afebf7329628616eb2c41c5
	BUILD_IN_SOURCE 0
	PREFIX ${CMAKE_BINARY_DIR}/deps/libmosquitto
	INSTALL_COMMAND ""
)

set(MOSQUITTO_INCLUDE_DIR ${CMAKE_BINARY_DIR}/deps/libmosquitto/src/libre/lib)

if (BUILD_PREFER_STATIC_LIBRARIES)
	if (APPLE)
		set(MOSQUITTO_LIBRARY ${CMAKE_BINARY_DIR}/deps/libmosquitto/src/libmosquitto-build/lib/libmosquitto.a)
	elseif(UNIX)
		set(MOSQUITTO_LIBRARY ${CMAKE_BINARY_DIR}/deps/libmosquitto/src/libmosquitto-build/lib/libmosquitto.a)
	elseif(WIN32)
		set(MOSQUITTO_LIBRARY ${CMAKE_BINARY_DIR}/deps/libmosquitto/src/libmosquitto-build/lib/libmosquitto.lib)
	else()
		message(FATAL_ERROR "Unknown platform!")
	endif()
else()
	if (APPLE)
		set(MOSQUITTO_LIBRARY ${CMAKE_BINARY_DIR}/deps/libmosquitto/src/libmosquitto-build/lib/libmosquitto.dylib)
	elseif(UNIX)
		set(MOSQUITTO_LIBRARY ${CMAKE_BINARY_DIR}/deps/libmosquitto/src/libmosquitto-build/lib/libmosquitto.so)
	elseif(WIN32)
		set(MOSQUITTO_LIBRARY ${CMAKE_BINARY_DIR}/deps/libmosquitto/src/libmosquitto-build/lib/libmosquitto.dll)
	else()
		message(FATAL_ERROR "Unknown platform!")
	endif()
endif()

set(MOSQUITTO_BUILT ON)
