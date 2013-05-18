# if you build swig from sources on windows, this is where it will end up
# see also: http://www.swig.org/Doc2.0/Windows.html#Windows_mingw_msys
if (WIN32)
	if(EXISTS "${PROJECT_BINARY_DIR}/../swig/")
		LIST(APPEND CMAKE_PROGRAM_PATH "${PROJECT_BINARY_DIR}/../swig/")
	elseif(EXISTS "${PROJECT_BINARY_DIR}/../../swig/")
		LIST(APPEND CMAKE_PROGRAM_PATH "${PROJECT_BINARY_DIR}/../../swig/")
	endif()

	LIST(APPEND CMAKE_PROGRAM_PATH "C:/Program Files/swig") # swig.exe
	LIST(APPEND CMAKE_PROGRAM_PATH "C:/Program Files (x86)/swig") # swig.exe
	#LIST(APPEND CMAKE_PROGRAM_PATH "C:/Windows/Microsoft.NET/Framework/v4.0.30319") # CSharp compiler
	LIST(APPEND CMAKE_PROGRAM_PATH "C:/Windows/Microsoft.NET/Framework/v3.5") # CSharp compiler
endif()

LIST(APPEND CMAKE_PROGRAM_PATH $ENV{SWIG_DIR})

FIND_PROGRAM(DMCS_EXECUTABLE dmcs PATHS $ENV{DMCS_HOME} ENV PATH ) # Mono compiler
FIND_PROGRAM(CSC_EXECUTABLE csc PATHS $ENV{CSC_HOME} ENV PATH ) # CSharp compiler

if (NOT IOS)
	find_package(SWIG)
	if (SWIG_FOUND)
		if(SWIG_VERSION VERSION_GREATER 2.0.4)
			MARK_AS_ADVANCED(SWIG_DIR SWIG_EXECUTABLE SWIG_VERSION)
			INCLUDE(${SWIG_USE_FILE})
			add_subdirectory(swig/java)
			if (NOT DIST_PREPARE)
				if (NOT WIN32)
					add_subdirectory(swig/python)
				endif()
				add_subdirectory(swig/perl)
				add_subdirectory(swig/php)
			endif()
			if (DMCS_EXECUTABLE OR CSC_EXECUTABLE)
				if (NOT ANDROID)
					add_subdirectory(swig/csharp)
				endif()
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
GET_TARGET_PROPERTY(UMUNDONATIVEJAVA_LOCATION umundoNativeJava LOCATION)
if (UMUNDONATIVEJAVA_LOCATION)

	#
	# TODO: We could move the protoc targets into s11n/CMakeLists.txt to
	# get rid of find_package(Protobuf REQUIRED) here
	#
	
	set(CMAKE_FIND_LIBRARY_SUFFIXES_ORIG ${CMAKE_FIND_LIBRARY_SUFFIXES})
	if (NOT 64BIT_HOST AND DIST_PREPARE)
	 	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_STATIC}) # link statically!
	endif()
	find_package(Protobuf REQUIRED)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_ORIG})

	# Find UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE
	include("${CMAKE_MODULE_PATH}/UseUMundo.cmake")

	set(JAR_EXCLUDE_DEBUG OFF)
	set(JAR_EXCLUDE_JNI OFF)

	# include all the JNI libraries prepared from DIST_PREPARE builds on the various desktop platforms
	if (DIST_PREPARE)
		if (CMAKE_CROSSCOMPILING)
			if (ANDROID)
				find_program(PROTOBUF_PROTOC_EXECUTABLE NAMES protoc protoc.exe)
				set(JAR_EXCLUDE_JNI ON) # JNI inside jar not allowed with Android
			endif()
			SET(JAR_JNI_ROOT_PATH ${PROJECT_SOURCE_DIR}/package/cross-compiled/${CMAKE_CROSSCOMPILING_TARGET_LC})
		else()
			SET(JAR_JNI_ROOT_PATH ${PROJECT_SOURCE_DIR}/package)
		endif()
	else()
		# when not preparing a distribution, just put the jar into the libs
		SET(JAR_JNI_ROOT_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
	endif()

	FIND_PROGRAM(ANT_EXECUTABLE ant PATHS $ENV{ANT_HOME}/bin ENV PATH )
	if (ANT_EXECUTABLE)
		# run protoc for java
#		ADD_CUSTOM_TARGET(java_protoc_s11n ALL
		ADD_CUSTOM_TARGET(java_protoc_s11n
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
		set_target_properties(java_protoc_s11n PROPERTIES FOLDER "Bindings")

#		ADD_CUSTOM_TARGET(java_protoc_rpc ALL
		ADD_CUSTOM_TARGET(java_protoc_rpc
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
		set_target_properties(java_protoc_rpc PROPERTIES FOLDER "Bindings")

		# we need to build the protoc plugin first
		if (UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE_DEP)
			add_dependencies(java_protoc_s11n ${UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE_DEP})
			add_dependencies(java_protoc_rpc ${UMUNDO_PROTOBUF_JAVA_RPC_EXECUTABLE_DEP})
		endif()

		# package everything into a jar
#		ADD_CUSTOM_TARGET(java_build_jar ALL
		ADD_CUSTOM_TARGET(java
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
		add_dependencies(java java_protoc_s11n)
		add_dependencies(java java_protoc_rpc)
		add_dependencies(java umundoNativeJava)
		set_target_properties(java PROPERTIES FOLDER "Bindings")
		if (BUILD_TESTS)
			add_dependencies(ALL_TESTS java)
		endif()

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

GET_TARGET_PROPERTY(UMUNDONATIVECSHARP_LOCATION umundoNativeCSharp LOCATION)
if (UMUNDONATIVECSHARP_LOCATION)
	if (CSC_EXECUTABLE)
		# package everything into a DLL
#		ADD_CUSTOM_TARGET(csharp_build_dll ALL
		ADD_CUSTOM_TARGET(csharp
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/TypedPublisher.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/ITypedReceiver.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/TypedSubscriber.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/GreeterDecorator.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/ITypedGreeter.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CSC_EXECUTABLE}
				/target:library
				/out:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundoCSharp${CMAKE_LIBRARY_POSTFIX}.dll
				/r:${PROJECT_SOURCE_DIR}/contrib/csharp/protobuf-net-r580/Full/net30/protobuf-net.dll
				*.cs
				WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMENT "Creating umundoCSharp${CMAKE_LIBRARY_POSTFIX}.dll for C# ...")
   add_dependencies(csharp umundoNativeCSharp)
		if (BUILD_TESTS)
			add_dependencies(ALL_TESTS csharp)
		endif()
		set_target_properties(csharp PROPERTIES FOLDER "Bindings")
	else()
		if (WIN32)
			message(STATUS "Could not find csc csharp compiler - will not build CSharp bindings")
		endif()
	endif()
	
	if (DMCS_EXECUTABLE)
		ADD_CUSTOM_TARGET(mono
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/TypedPublisher.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/ITypedReceiver.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/TypedSubscriber.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/GreeterDecorator.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${CMAKE_COMMAND} -E 
				copy ${PROJECT_SOURCE_DIR}/s11n/src/umundo-csharp/ITypedGreeter.cs ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMAND ${DMCS_EXECUTABLE}
				-target:library
				/out:${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/umundoMono${CMAKE_LIBRARY_POSTFIX}.dll
				/r:${PROJECT_SOURCE_DIR}/contrib/csharp/protobuf-net-r580/Full/net30/protobuf-net.dll
				*.cs
				WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/core/bindings/swig/csharp/org/umundo/core
			COMMENT "Creating the library for Mono ...")
   add_dependencies(mono umundoNativeCSharp)
		if (BUILD_TESTS)
			add_dependencies(ALL_TESTS mono)
		endif()
		set_target_properties(mono PROPERTIES FOLDER "Bindings")
	endif()
	
endif()
