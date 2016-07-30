
##############################################################################
# Provide custom install_* macros to account for all files
##############################################################################

include(CMakeParseArguments)

function(INSTALL_HEADERS)
	set(options)
	set(oneValueArgs COMPONENT)
	set(multiValueArgs HEADERS)
	cmake_parse_arguments(INSTALL_HEADERS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	FOREACH(HEADER ${INSTALL_HEADERS_HEADERS})
		# message(STATUS "ADDING HEADER ${HEADER}")
		if (${HEADER} MATCHES "${CMAKE_BINARY_DIR}.*")
			STRING(REGEX REPLACE "${CMAKE_BINARY_DIR}" "" REL_HEADER ${HEADER})
			STRING(REGEX MATCH "[^/\\](.*)[/\\]" REL_HEADER ${REL_HEADER})
			SET(REL_HEADER "umundo/${REL_HEADER}")
			# message(STATUS "MATCHED CMAKE_BINARY_DIR -> ${REL_HEADER}")
		elseif(${HEADER} MATCHES "${PROJECT_SOURCE_DIR}.*")
			STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}" "" REL_HEADER ${HEADER})
			# STRING(REGEX MATCH "umundo(.*)[/\\]" REL_HEADER ${REL_HEADER})
			# message(STATUS "MATCHED PROJECT_SOURCE_DIR -> ${REL_HEADER}")
		else()
			message(STATUS "MATCHED no known prefix: ${HEADER}")
		endif()
		STRING(REGEX MATCH "(.*)[/\\]" DIR ${REL_HEADER})
		if (NOT DIR)
			message("Refusing to add header file ${REL_HEADER} for include in uppermost directory")
		else()
#			message("ADDING ${HEADER} in include/${DIR} for ${INSTALL_HEADERS_COMPONENT}")
			INSTALL(FILES ${HEADER} DESTINATION include/${DIR} COMPONENT ${INSTALL_HEADERS_COMPONENT})
		endif()
	ENDFOREACH()
endfunction()

function(INSTALL_FILES)
	set(options)
	set(oneValueArgs COMPONENT DESTINATION)
	set(multiValueArgs FILES)
	cmake_parse_arguments(INSTALL_FILE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	install(FILES ${INSTALL_FILE_FILES} DESTINATION ${INSTALL_FILE_DESTINATION} COMPONENT ${INSTALL_FILE_COMPONENT})
endfunction()

function(INSTALL_LIBRARY)
	set(options)
	set(oneValueArgs COMPONENT DESTINATION)
	set(multiValueArgs TARGETS)
	cmake_parse_arguments(INSTALL_LIBRARY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	if (NOT INSTALL_LIBRARY_DESTINATION)
		set(INSTALL_LIBRARY_DESTINATION lib)
	endif()
	install(TARGETS ${INSTALL_LIBRARY_TARGETS} DESTINATION ${INSTALL_LIBRARY_DESTINATION} COMPONENT ${INSTALL_LIBRARY_COMPONENT})
endfunction()

function(INSTALL_EXECUTABLE)
	set(options)
	set(oneValueArgs COMPONENT)
	set(multiValueArgs TARGETS)
	cmake_parse_arguments(INSTALL_EXECUTABLE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	install(
		TARGETS ${INSTALL_EXECUTABLE_TARGETS} 
		DESTINATION bin
		COMPONENT ${INSTALL_EXECUTABLE_COMPONENT}
		PERMISSIONS WORLD_EXECUTE OWNER_EXECUTE GROUP_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
endfunction()

# see http://www.cmake.org/Wiki/CMakeCompareVersionStrings
SET(THREE_PART_VERSION_REGEX "[0-9]+\\.[0-9]+\\.[0-9]+")
SET(TWO_PART_VERSION_REGEX "[0-9]+\\.[0-9]+")

# Breaks up a string in the form n1.n2.n3 into three parts and stores
# them in major, minor, and patch.  version should be a value, not a
# variable, while major, minor and patch should be variables.

MACRO(THREE_PART_VERSION_TO_VARS version major minor patch)
  IF(${version} MATCHES ${THREE_PART_VERSION_REGEX})
    STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" ${major} "${version}")
    STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" ${minor} "${version}")
    STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" ${patch} "${version}")
  ELSEIF(${version} MATCHES ${TWO_PART_VERSION_REGEX})
    STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+" "\\1" ${major} "${version}")
    STRING(REGEX REPLACE "^[0-9]+\\.([0-9])+" "\\1" ${minor} "${version}")
  ELSE(${version} MATCHES ${THREE_PART_VERSION_REGEX})
    MESSAGE("MACRO(THREE_PART_VERSION_TO_VARS ${version} ${major} ${minor} ${patch}")
    MESSAGE(FATAL_ERROR "Problem parsing version string, I can't parse it properly.")
  ENDIF(${version} MATCHES ${THREE_PART_VERSION_REGEX})
ENDMACRO(THREE_PART_VERSION_TO_VARS)

include(CheckCXXCompilerFlag)
MACRO(CHECK_AND_ADD_CXX_COMPILER_FLAG flag)
	set(FLAG_NAME ${flag})
	string(TOUPPER ${FLAG_NAME} FLAG_NAME)
	string(REGEX REPLACE "^-" "" FLAG_NAME ${FLAG_NAME})
	string(REGEX REPLACE "-" "_" FLAG_NAME ${FLAG_NAME})
	string(REGEX REPLACE "=.*$" "" FLAG_NAME ${FLAG_NAME})
	CHECK_CXX_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_FLAG_${FLAG_NAME})
	if(COMPILER_SUPPORTS_FLAG_${FLAG_NAME})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
	endif()
ENDMACRO()

include(CheckCCompilerFlag)
MACRO(CHECK_AND_ADD_C_COMPILER_FLAG flag)
	set(FLAG_NAME ${flag})
	string(TOUPPER ${FLAG_NAME} FLAG_NAME)
	string(REGEX REPLACE "^-" "" FLAG_NAME ${FLAG_NAME})
	string(REGEX REPLACE "-" "_" FLAG_NAME ${FLAG_NAME})
	string(REGEX REPLACE "=.*$" "" FLAG_NAME ${FLAG_NAME})
	CHECK_C_COMPILER_FLAG(${flag} COMPILER_SUPPORTS_FLAG_${FLAG_NAME})
	if(COMPILER_SUPPORTS_FLAG_${FLAG_NAME})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
	endif()
ENDMACRO()

MACRO(CHECK_AND_ADD_COMPILER_FLAG flag)
	CHECK_AND_ADD_C_COMPILER_FLAG(${flag})
	CHECK_AND_ADD_CXX_COMPILER_FLAG(${flag})
ENDMACRO()
