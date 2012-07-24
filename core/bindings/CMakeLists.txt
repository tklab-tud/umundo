# if you build swig from sources on windows, this is where it will end up
# see also: http://www.swig.org/Doc2.0/Windows.html#Windows_mingw_msys
if (WIN32)
	LIST(APPEND CMAKE_PROGRAM_PATH "C:/MinGW/msys/1.0/src/swig") # swig.exe
	LIST(APPEND CMAKE_PROGRAM_PATH "C:/Windows/Microsoft.NET/Framework/v3.5") # csc.exe
endif()
LIST(APPEND CMAKE_PROGRAM_PATH $ENV{SWIG_DIR})

if (NOT IOS)
	find_package(SWIG)
	if (SWIG_FOUND)
		if(SWIG_VERSION VERSION_GREATER 2.0.4)
			MARK_AS_ADVANCED(SWIG_DIR SWIG_EXECUTABLE SWIG_VERSION)
			INCLUDE(${SWIG_USE_FILE})
			add_subdirectory(swig/java)
			if (WIN32)
				add_subdirectory(swig/csharp)
			endif()
		else()
			message("SWIG version 2.0.5 is required, found ${SWIG_VERSION} - skipping java wrapper generation")
		endif()
	else()
		message("SWIG not found - skipping wrapper generation")
	endif()
endif()

############################################################
# JAVA: run protoc and build jar
############################################################

# only run ant if we actually build the java bindings
GET_TARGET_PROPERTY(UMUNDOCOREJAVA_LOCATION umundocoreJava LOCATION)
if (UMUNDOCOREJAVA_LOCATION)
	# Find UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE
	include("${CMAKE_MODULE_PATH}/UseUMundo.cmake")

	set(JAR_EXCLUDE_DEBUG OFF)
	set(JAR_EXCLUDE_JNI OFF)
	
	# include all the JNI libraries prepared from DIST_PREPARE builds on the various desktop platforms
	if (CMAKE_CROSSCOMPILING)
		SET(JAR_JNI_ROOT_PATH ${PROJECT_SOURCE_DIR}/package/cross-compiled/${CMAKE_CROSSCOMPILING_TARGET_LC}-${CMAKE_SYSTEM_VERSION})
		if (ANDROID)
			find_program(PROTOBUF_PROTOC_EXECUTABLE NAMES protoc protoc.exe)
			set(JAR_EXCLUDE_JNI ON)
		endif()
	else()
		if (DIST_PREPARE)
			SET(JAR_JNI_ROOT_PATH ${PROJECT_SOURCE_DIR}/package)
		else()
			SET(JAR_JNI_ROOT_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
		endif()
	endif()
	
	FIND_PROGRAM(ANT_EXECUTABLE ant PATHS $ENV{ANT_HOME} ENV PATH )
	if (ANT_EXECUTABLE)
		# run protoc for java
		ADD_CUSTOM_TARGET(java_protoc_s11n ALL
			COMMAND ${ANT_EXECUTABLE} 
			-Dsrc.dir=${PROJECT_SOURCE_DIR} 
			-Dbuild.dir=${PROJECT_BINARY_DIR} 
			-Dbuild.type=${CMAKE_BUILD_TYPE} 
			-Dprotoc.src.dir=${PROJECT_SOURCE_DIR}/s11n/interfaces/protobuf/
			-Dprotoc.plugin=${UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE}
			-Dprotoc.bin=${PROTOBUF_PROTOC_EXECUTABLE}
			-f build-java.xml protoc
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib/java
			COMMENT "Creating ProtoBuf messages for Java ...")
		add_dependencies(java_protoc_s11n umundocoreJava)
		set_target_properties(java_protoc_s11n PROPERTIES FOLDER "Bindings")

		ADD_CUSTOM_TARGET(java_protoc_rpc ALL
			COMMAND ${ANT_EXECUTABLE} 
			-Dsrc.dir=${PROJECT_SOURCE_DIR} 
			-Dbuild.dir=${PROJECT_BINARY_DIR} 
			-Dbuild.type=${CMAKE_BUILD_TYPE} 
			-Dprotoc.src.dir=${PROJECT_SOURCE_DIR}/rpc/services/protobuf/
			-Dprotoc.plugin=${UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE}
			-Dprotoc.bin=${PROTOBUF_PROTOC_EXECUTABLE}
			-f build-java.xml protoc
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib/java
			COMMENT "Creating ProtoBuf services for Java ...")
		add_dependencies(java_protoc_rpc umundocoreJava)
		set_target_properties(java_protoc_rpc PROPERTIES FOLDER "Bindings")

		# we need to build the protoc plugin first
		if (UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE_DEP)
			add_dependencies(java_protoc_s11n ${UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE_DEP})
			add_dependencies(java_protoc_rpc ${UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE_DEP})
		endif()

		# package everything into a jar
		ADD_CUSTOM_TARGET(java_build_jar ALL
			COMMAND ${ANT_EXECUTABLE} 
			-Dlib.dir=${JAR_JNI_ROOT_PATH} 
			-Dsrc.dir=${PROJECT_SOURCE_DIR} 
			-Dbuild.dir=${PROJECT_BINARY_DIR} 
			-Dbuild.type=${CMAKE_BUILD_TYPE} 
			-Dexclude.debug=${JAR_EXCLUDE_DEBUG}
			-Dexclude.jni=${JAR_EXCLUDE_JNI}
			-f build-java.xml
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/contrib/java
			COMMENT "Creating the jar ...")
		# make sure ant runs after we built the wrappers
		add_dependencies(java_build_jar java_protoc_s11n)
		add_dependencies(java_build_jar java_protoc_rpc)
		add_dependencies(java_build_jar umundocoreJava)
		set_target_properties(java_build_jar PROPERTIES FOLDER "Bindings")

		add_test(java-tests 
			${ANT_EXECUTABLE} 
			-Dlib.dir=${JAR_JNI_ROOT_PATH} 
			-Dsrc.dir=${PROJECT_SOURCE_DIR} 
			-Dbuild.dir=${PROJECT_BINARY_DIR} 
			-Dbuild.type=${CMAKE_BUILD_TYPE} 
			-f ${PROJECT_SOURCE_DIR}/contrib/java/build-java.xml 
			test)
	else()
		message(STATUS "Could not find ant binary - will not build jars")
	endif()
endif()

############################################################
# CSHARP: run protoc and build dll
############################################################

GET_TARGET_PROPERTY(UMUNDOCORECSHARP_LOCATION umundocoreCSharp LOCATION)
if (UMUNDOCORECSHARP_LOCATION)
	FIND_PROGRAM(CSC_EXECUTABLE csc PATHS $ENV{CSC_HOME} ENV PATH )
	if (CSC_EXECUTABLE)
		# package everything into a jar
		ADD_CUSTOM_TARGET(csharp_build_dll ALL
			COMMAND ${CSC_EXECUTABLE} 
			/target:library
			/out:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundoCSharp.dll
			*.cs
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMENT "Creating the dll for C# ...")
		# make sure ant runs after we built the wrappers
		add_dependencies(csharp_build_dll umundocoreCSharp)
		set_target_properties(java_build_jar PROPERTIES FOLDER "Bindings")
	else()
		message(STATUS "Could not find csc binary - will not build CSharp dll")
	endif()
endif()

# add objective-c bindings on mac and ios
if (APPLE OR IOS)
	file(GLOB_RECURSE UMUNDOCORE_OBJC_FILES umundo-objc/core/*.mm)
	list(APPEND UMUNDOCORE_FILES ${UMUNDOCORE_OBJC_FILES})
	file(GLOB_RECURSE UMUNDOCORE_OBJC_HEADER_FILES bindings/umundo-objc/core/*.h)
	list(APPEND UMUNDOCORE_HEADER_FILES ${UMUNDOCORE_OBJC_HEADER_FILES})
	if (IOSSIM)
		foreach(UMUNDOCORE_OBJC_FILE ${UMUNDOCORE_OBJC_FILES})
			set_source_files_properties(${UMUNDOCORE_OBJC_FILE} PROPERTIES COMPILE_FLAGS "-fobjc-abi-version=2 -fobjc-legacy-dispatch")
		endforeach()
	endif()
	include_directories(.)
	set(UMUNDOCORE_FILES "${UMUNDOCORE_FILES}" PARENT_SCOPE)
	set(UMUNDOCORE_HEADER_FILES "${UMUNDOCORE_HEADER_FILES}" PARENT_SCOPE)
endif()
