# see: http://www.vtk.org/Wiki/CMake:CPackConfiguration

########################################################################################
# gather host-native libraries
################################################################################

# these are all the host-native libraries plus
file(GLOB_RECURSE PLATFORM_LIBS
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.a
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.so
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.lib
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.dylib
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.jnilib
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.dll
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.pdb
#	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.exp
)

# sort host-native libraries into installation components
foreach(PLATFORM_LIB ${PLATFORM_LIBS})
#	message("PLATFORM_LIB: ${PLATFORM_LIB}")
	if (PLATFORM_LIB MATCHES ".*Native.*")
		install(FILES ${PLATFORM_LIB} DESTINATION lib COMPONENT librarySwig)
		list (APPEND UMUNDO_CPACK_COMPONENTS "librarySwig")
	elseif (PLATFORM_LIB MATCHES ".*umundoCSharp.*")
		install(FILES ${PLATFORM_LIB} DESTINATION share/umundo/lib COMPONENT librarySwig)
		list (APPEND UMUNDO_CPACK_COMPONENTS "library")
	elseif (PLATFORM_LIB MATCHES ".*umundoserial.*")
		install(FILES ${PLATFORM_LIB} DESTINATION lib COMPONENT library)
		list (APPEND UMUNDO_CPACK_COMPONENTS "library")
	elseif (PLATFORM_LIB MATCHES ".*umundocore.*")
		install(FILES ${PLATFORM_LIB} DESTINATION lib COMPONENT library)
		list (APPEND UMUNDO_CPACK_COMPONENTS "library")
	elseif (PLATFORM_LIB MATCHES ".*umundorpc.*")
		install(FILES ${PLATFORM_LIB} DESTINATION lib COMPONENT library)
		list (APPEND UMUNDO_CPACK_COMPONENTS "library")
	elseif (PLATFORM_LIB MATCHES ".*umundoutil.*")
		install(FILES ${PLATFORM_LIB} DESTINATION lib COMPONENT library)
		list (APPEND UMUNDO_CPACK_COMPONENTS "library")
	elseif (PLATFORM_LIB MATCHES ".*umundo.*")
		install(FILES ${PLATFORM_LIB} DESTINATION lib COMPONENT library)
		list (APPEND UMUNDO_CPACK_COMPONENTS "library")
	else()
		message(STATUS "PACKAGE RELEASE UNK ${PLATFORM_LIB} - not packaging")
	endif()
endforeach()

########################################
# Pre-built libraries for host platform
########################################

# do follow symlinks with GLOB_RECURSE
#cmake_policy(SET CMP0009 OLD)

file(GLOB_RECURSE PREBUILT_LIBS FOLLOW_SYMLINKS
	${UMUNDO_PREBUILT_LIBRARY_PATH}/lib/*.a
	${UMUNDO_PREBUILT_LIBRARY_PATH}/lib/*.so
	${UMUNDO_PREBUILT_LIBRARY_PATH}/lib/*.dylib
	${UMUNDO_PREBUILT_LIBRARY_PATH}/lib/*.lib
	${UMUNDO_PREBUILT_LIBRARY_PATH}/lib/*.dll
	${UMUNDO_PREBUILT_LIBRARY_PATH}/lib/*.pdb
)

#message("UMUNDO_PREBUILT_LIBRARY_PATH: ${UMUNDO_PREBUILT_LIBRARY_PATH}")

foreach(PREBUILT_LIB ${PREBUILT_LIBS})
#	message("PREBUILT_LIB: ${PREBUILT_LIB}")
	# string(REGEX MATCH "prebuilt/[^//]+/[^//]+" CURR_PLATFORM ${PREBUILT_LIB})
	# message("CURR_PLATFORM: ${CURR_PLATFORM}")
	# install(FILES ${PREBUILT_LIB} DESTINATION share/umundo/${CURR_PLATFORM} COMPONENT libraryPrebuilt)
	install(FILES ${PREBUILT_LIB} DESTINATION share/umundo/prebuilt COMPONENT libraryPrebuilt)
	list (APPEND UMUNDO_CPACK_COMPONENTS "libraryPrebuilt")
endforeach()


########################################
# Include documentation
########################################

# file(GLOB_RECURSE HTML_DOCS ${PROJECT_SOURCE_DIR}/docs/html/*)
# foreach(HTML_DOC ${HTML_DOCS})
# 	STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/" "" HTML_PATH ${HTML_DOC})
# 	STRING(REGEX MATCH "(.*)[/\\]" HTML_PATH ${HTML_PATH})
# 	install(FILES ${HTML_DOC} DESTINATION share/umundo/${HTML_PATH} COMPONENT docs)
# 	list (APPEND UMUNDO_CPACK_COMPONENTS "docs")
# #	message(STATUS ${HTML_PATH})
# endforeach()

########################################
# Target languages
########################################

GET_TARGET_PROPERTY(UMUNDONATIVEJAVA_LOCATION umundoNativeJava LOCATION)
if (UMUNDONATIVEJAVA_LOCATION)
	if (DIST_PREPARE)
		if (EXISTS "${PROJECT_SOURCE_DIR}/package/umundo.jar")
			install(FILES ${PROJECT_SOURCE_DIR}/package/umundo.jar DESTINATION share/umundo/lib COMPONENT librarySwig)
		endif()
	else()
		if (EXISTS "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundo.jar")
			install(FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundo.jar DESTINATION share/umundo/lib COMPONENT librarySwig)
		endif()
	endif()
	list (APPEND UMUNDO_CPACK_COMPONENTS "librarySwig")
endif()

# The CSharp bindings are already picked up as a host-native dll above
GET_TARGET_PROPERTY(UMUNDONATIVECSHARP_LOCATION umundoNativeCSharp LOCATION)
# if (UMUNDONATIVECSHARP_LOCATION)
# 	install(FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundoCSharp.dll DESTINATION share/umundo/lib COMPONENT librarySwig)
# 	list (APPEND UMUNDO_CPACK_COMPONENTS "librarySwig")
# endif()


################################################################################
# Cross Compiled binaries
################################################################################

########################################
# Android
########################################

file(GLOB_RECURSE ANDROID_LIBS ${PROJECT_SOURCE_DIR}/package/cross-compiled/android*/*)
foreach(ANDROID_LIB ${ANDROID_LIBS})
	# do not pack static libraries
#	if (NOT ANDROID_LIB MATCHES ".*\\.a" AND NOT ANDROID_LIB MATCHES "\\..*")
	if (NOT ANDROID_LIB MATCHES ".*\\.a")
		# remove weird double slashes
		STRING(REGEX REPLACE "//" "/" ANDROID_LIB ${ANDROID_LIB})
		# take relative path
		STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/package/cross-compiled/" "" ANDROID_PATH ${ANDROID_LIB})
    # only take first two path elements
		STRING(REGEX MATCH "[^/]*/[^/]*" ANDROID_PATH ${ANDROID_PATH})
		# but remove umundo.jar from path in any case
		STRING(REGEX REPLACE "/umundo.jar" "" ANDROID_PATH ${ANDROID_PATH})
    # message(STATUS "ANDROID_PATH: ${ANDROID_PATH}")
    # message(STATUS "ANDROID_LIB: ${ANDROID_LIB}")
		install(FILES ${ANDROID_LIB} DESTINATION share/umundo/${ANDROID_PATH} COMPONENT libraryAndroid)
		list (APPEND UMUNDO_CPACK_COMPONENTS "libraryAndroid")
	endif()
endforeach()

# list(FIND UMUNDO_CPACK_COMPONENTS "libraryAndroid" FOUND_ITEM)
# if (FOUND_ITEM GREATER -1)
# 	file(GLOB_RECURSE ANDROID_PREBUILT_LIBS ${PROJECT_SOURCE_DIR}/contrib/prebuilt/android*/*)
# 	foreach(ANDROID_PREBUILT_LIB ${ANDROID_PREBUILT_LIBS})
# 		STRING(REGEX REPLACE "//" "/" ANDROID_PREBUILT_LIB ${ANDROID_PREBUILT_LIB})
# 		STRING(REGEX MATCH "prebuilt/[^//]+/[^//]+" ANDROID_PLATFORM ${ANDROID_PREBUILT_LIB})
# 		message("ANDROID_PREBUILT_LIB: ${ANDROID_PREBUILT_LIB}")
# 		message("ANDROID_PLATFORM: ${ANDROID_PLATFORM}")
# 		install(FILES ${ANDROID_PREBUILT_LIB} DESTINATION share/umundo/${ANDROID_PLATFORM} COMPONENT libraryPrebuilt)
# 	endforeach()
# endif()

########################################
# iOS
########################################
if (APPLE)
	file(GLOB_RECURSE IOS_LIBS ${PROJECT_SOURCE_DIR}/package/cross-compiled/ios*/*.ios*)
	foreach(IOS_LIB ${IOS_LIBS})
		# match ios-5.0
		STRING(REGEX REPLACE "//" "/" IOS_LIB ${IOS_LIB})
		STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/package/cross-compiled/" "" IOS_PATH ${IOS_LIB})
		STRING(REGEX MATCH "[^/]*" IOS_PATH ${IOS_PATH})
		# message(STATUS "IOS_LIB:  ${IOS_LIB}")
		# message(STATUS "IOS_PATH: ${IOS_PATH}")
		install(FILES ${IOS_LIB} DESTINATION share/umundo/${IOS_PATH} COMPONENT libraryIOS)
		list (APPEND UMUNDO_CPACK_COMPONENTS "libraryIOS")
	endforeach()

	list(FIND UMUNDO_CPACK_COMPONENTS "libraryIOS" FOUND_ITEM)
	if (FOUND_ITEM GREATER -1)
		file(GLOB_RECURSE IOS_PREBUILT_LIBS ${PROJECT_SOURCE_DIR}/contrib/prebuilt/ios*/*.a)
		foreach(IOS_PREBUILT_LIB ${IOS_PREBUILT_LIBS})
			if(NOT EXISTS "${IOS_PREBUILT_LIB}/")
				STRING(REGEX REPLACE "//" "/" IOS_PREBUILT_LIB ${IOS_PREBUILT_LIB})
				STRING(REGEX MATCH "prebuilt/[^//]+/[^//]+" IOS_PLATFORM ${IOS_PREBUILT_LIB})
				# message("IOS_PLATFORM: ${IOS_PLATFORM}")
				# message("IOS_PREBUILT_LIB: ${IOS_PREBUILT_LIB}")
				install(FILES ${IOS_PREBUILT_LIB} DESTINATION share/umundo/${IOS_PLATFORM} COMPONENT libraryPrebuilt)
			endif()
		endforeach()
	endif()
endif()

################################################################################
# Sample projects
################################################################################

# umundo-pingpong: XCode for iOS
if (APPLE)
	file(GLOB_RECURSE IOS_PINGPONG_SAMPLE ${PROJECT_SOURCE_DIR}/contrib/samples/ios/umundo-pingpong*/*)
	foreach(IOS_PINGPONG_SAMPLE_FILE ${IOS_PINGPONG_SAMPLE})
		# strip root
		STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/contrib/samples" "" REL_PATH ${IOS_PINGPONG_SAMPLE_FILE})
		get_filename_component(REL_PATH ${REL_PATH} PATH)
#		message("Installing ${IOS_PINGPONG_SAMPLE_FILE} in share/umundo/samples/${REL_PATH}")
		install(FILES ${IOS_PINGPONG_SAMPLE_FILE} DESTINATION share/umundo/samples/${REL_PATH} COMPONENT samples)
	endforeach()
	list (APPEND UMUNDO_CPACK_COMPONENTS "samples")
endif()

# umundo-pingpong: Eclipse for Android
file(GLOB_RECURSE ANDROID_PINGPONG_SAMPLE ${PROJECT_SOURCE_DIR}/contrib/samples/android/*)
foreach(ANDROID_PINGPONG_SAMPLE_FILE ${ANDROID_PINGPONG_SAMPLE})
#	message("ANDROID_PINGPONG_SAMPLE_FILE: ${ANDROID_PINGPONG_SAMPLE_FILE}")
	STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/contrib/samples" "" REL_PATH ${ANDROID_PINGPONG_SAMPLE_FILE})
	get_filename_component(REL_PATH ${REL_PATH} PATH)
	install(FILES ${ANDROID_PINGPONG_SAMPLE_FILE} DESTINATION share/umundo/samples/${REL_PATH} COMPONENT samples)
endforeach()
list (APPEND UMUNDO_CPACK_COMPONENTS "samples")

# umundo-pingpong: Visual Studio for CSharp
if (WIN32)
	file(GLOB_RECURSE CSHARP_PINGPONG_SAMPLE ${PROJECT_SOURCE_DIR}/contrib/samples/csharp/*)
	foreach(CSHARP_PINGPONG_SAMPLE_FILE ${CSHARP_PINGPONG_SAMPLE})
#		message("CSHARP_PINGPONG_SAMPLE_FILE: ${CSHARP_PINGPONG_SAMPLE_FILE}")
		STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/contrib/samples" "" REL_PATH ${CSHARP_PINGPONG_SAMPLE_FILE})
		get_filename_component(REL_PATH ${REL_PATH} PATH)
		install(FILES ${CSHARP_PINGPONG_SAMPLE_FILE} DESTINATION share/umundo/samples/${REL_PATH} COMPONENT samples)
	endforeach()
	list (APPEND UMUNDO_CPACK_COMPONENTS "samples")
endif()

# All the java samples
file(GLOB_RECURSE JAVA_SAMPLES ${PROJECT_SOURCE_DIR}/contrib/samples/java/*)
foreach(JAVA_SAMPLES_FILE ${JAVA_SAMPLES})
	STRING(REGEX REPLACE "${PROJECT_SOURCE_DIR}/contrib/samples" "" REL_PATH ${JAVA_SAMPLES_FILE})
	get_filename_component(REL_PATH ${REL_PATH} PATH)
	install(FILES ${JAVA_SAMPLES_FILE} DESTINATION share/umundo/samples/${REL_PATH} COMPONENT samples)
endforeach()

list (APPEND UMUNDO_CPACK_COMPONENTS "samples")

########################################
# House keeping
########################################

list (APPEND UMUNDO_CPACK_COMPONENTS "headers")

if (NOT CMAKE_CROSS_COMPILING)
	list (APPEND UMUNDO_CPACK_COMPONENTS "tools")
endif()
list (REMOVE_DUPLICATES UMUNDO_CPACK_COMPONENTS ${UMUNDO_CPACK_COMPONENTS})
#message("UMUNDO_CPACK_COMPONENTS: ${UMUNDO_CPACK_COMPONENTS}")

########################################
# Configure packagers
########################################

if (UNIX)
	if (APPLE)
		set(CPACK_GENERATOR "PackageMaker;TGZ")
	else()
		set(CPACK_GENERATOR "DEB;RPM;TGZ")
	endif()
	set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
endif()
if (WIN32)
	set(CPACK_GENERATOR "NSIS;ZIP")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "uMundo")
	# pairs of executables and labels for start menu
	set(CPACK_NSIS_MENU_LINKS
		" ;uMundo SDK"
		"bin\\\\umundo-pingpong.exe;uMundo Ping-Pong"
		"bin\\\\umundo-monitor.exe;uMundo Monitor")

endif()

set(CPACK_PACKAGE_NAME "umundo")
set(CPACK_PACKAGE_VENDOR "Telecooperation Group - TU Darmstadt")
set(CPACK_PACKAGE_CONTACT "radomski@tk.informatik.tu-darmstadt.de")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "uMundo - publish/subscribe since 2012")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/installer/description.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/installer/license.txt")

set(CPACK_PACKAGE_VERSION "${UMUNDO_VERSION_MAJOR}.${UMUNDO_VERSION_MINOR}.${UMUNDO_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR ${UMUNDO_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${UMUNDO_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${UMUNDO_VERSION_PATCH})

if (64BIT_HOST AND WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}_64-${CPACK_PACKAGE_VERSION}")
else()
	set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_SYSTEM_NAME_LC}-${CMAKE_SYSTEM_PROCESSOR}-${CPACK_PACKAGE_VERSION}")
endif()

###
# Configuration for NSIS installer on Win32
#
# pairs of executables and labels for start menu
#CPACK_PACKAGE_EXECUTABLES
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "umundo.telecooperation.tu-darmstadt")
if (WIN32)
	set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}\\\\installer\\\\nsis\\\\umundo-logo.bmp")
else()
	set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/installer/nsis/umundo-logo.bmp")
endif()

###
# Configuration for PackageMaker on MacOSX
#
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/installer/packageMaker/readme.txt")
set(CPACK_RESOURCE_FILE_WELCOME "${PROJECT_SOURCE_DIR}/installer/packageMaker/welcome.txt")

###
# Configuration for debian packages
#
set(CPACK_DEBIAN_PACKAGE_NAME "umundo")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libavahi-client3")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "swig2.0, protobuf-compiler")

###
# Configuration for RPM packages
#
set(CPACK_RPM_PACKAGE_NAME "umundo")
set(CPACK_RPM_PACKAGE_LICENSE "CDDL")


########################################
# Describe layout of package
########################################

set(CPACK_COMPONENTS_ALL
	${UMUNDO_CPACK_COMPONENTS}
)

###
# Description of components
#

list(FIND UMUNDO_CPACK_COMPONENTS "tools" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	set(CPACK_COMPONENT_TOOLS_DISPLAY_NAME "Command-line Tools")
	set(CPACK_COMPONENT_TOOLS_DESCRIPTION "Command-line tools to debug and monitor a umundo network.")
endif()

list(FIND UMUNDO_CPACK_COMPONENTS "samples" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	set(CPACK_COMPONENT_SAMPLES_DISPLAY_NAME "IDE Templates and sample programs")
	set(CPACK_COMPONENT_SAMPLES_DESCRIPTION
  		"Templates for Xcode, Visual Studio and Eclipse with sample programs.")
endif()

list(FIND UMUNDO_CPACK_COMPONENTS "docs" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	set(CPACK_COMPONENT_DOCS_DISPLAY_NAME "Documentation")
	set(CPACK_COMPONENT_DOCS_DESCRIPTION "Auto-generated documentation.")
endif()

list(FIND UMUNDO_CPACK_COMPONENTS "librarySwig" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	set(CPACK_COMPONENT_LIBRARYSWIG_DISPLAY_NAME "Java interface")
	set(CPACK_COMPONENT_LIBRARYSWIG_DESCRIPTION "umundo.core library wrapped for Java per native interfaces. This will install the actual library and the JAR archive.")
	set(CPACK_COMPONENT_LIBRARYSWIG_GROUP "Development")
endif()

list(FIND UMUNDO_CPACK_COMPONENTS "libraryPrebuilt" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	set(CPACK_COMPONENT_LIBRARYPREBUILT_DISPLAY_NAME "C++ dependent libraries")
	set(CPACK_COMPONENT_LIBRARYPREBUILT_DESCRIPTION "Prebuilt libraries for this host and the cross-compile targets")
	set(CPACK_COMPONENT_LIBRARYPREBUILT_GROUP "Development")
endif()

list(FIND UMUNDO_CPACK_COMPONENTS "libraryAndroid" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	set(CPACK_COMPONENT_LIBRARYANDROID_DISPLAY_NAME "Android libraries")
	set(CPACK_COMPONENT_LIBRARYANDROID_DESCRIPTION "umundo.core cross compiled for Android devices.")
	set(CPACK_COMPONENT_LIBRARYANDROID_GROUP "Development")
endif()

list(FIND UMUNDO_CPACK_COMPONENTS "libraryIOS" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	set(CPACK_COMPONENT_LIBRARYIOS_DISPLAY_NAME "iOS libraries")
	set(CPACK_COMPONENT_LIBRARYIOS_DESCRIPTION "umundo.core cross compiled for iOS devices (universal libraries).")
	set(CPACK_COMPONENT_LIBRARYIOS_GROUP "Development")
	set(CPACK_COMPONENT_LIBRARYIOS_DEPENDS headers)
	set(CPACK_COMPONENT_LIBRARYIOS_DEPENDS libraryPrebuilt)
endif()

list(FIND UMUNDO_CPACK_COMPONENTS "library" FOUND_ITEM)
if (FOUND_ITEM GREATER -1)
	# define header description here as well
	set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ headers ")
	set(CPACK_COMPONENT_HEADERS_DESCRIPTION "C++ header files for umundo and all its components.")
	set(CPACK_COMPONENT_HEADERS_GROUP "Development")

	set(CPACK_COMPONENT_LIBRARY_DISPLAY_NAME "C++ umundo libraries")
	set(CPACK_COMPONENT_LIBRARY_DESCRIPTION "Static libraries of the umundo components for C++ development.")
	set(CPACK_COMPONENT_LIBRARY_GROUP "Development")
	set(CPACK_COMPONENT_LIBRARY_DEPENDS headers)
	set(CPACK_COMPONENT_LIBRARY_DEPENDS libraryPrebuilt)
endif()

set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION "Libraries and Headers for umundo.")
