if (CMAKE_CROSSCOMPILING)
	return()
endif()

find_package(PHP5)
if(NOT PHP5_FOUND)
	return()
endif()

# copied from cmake's FindPHP4.cmake and adapted
if(APPLE)
	# this is a hack for now
  set(PHP_MODULE_CXX_FLAGS "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -Wl,-flat_namespace")
  foreach(symbol
		__efree
		__emalloc
		__estrdup
		__estrndup
		__object_init
		__object_init_ex
		__zend_get_parameters_array_ex
		__zend_list_find
		__zval_copy_ctor
		__zval_copy_ctor_func
		_add_property_zval_ex
		_alloc_globals
		_compiler_globals
		_convert_to_double
		_convert_to_long
		_convert_to_boolean
		__convert_to_string
		_gc_remove_zval_from_buffer
		_call_user_function
		_executor_globals
		_zend_get_constant
		_zend_lookup_class
		_zend_register_long_constant
		_zval_is_true
		_zend_error
		_zend_hash_find
		__zend_hash_add_or_update
		_zend_register_internal_class_ex
		_zend_register_list_destructors_ex
		_zend_register_resource
		_zend_rsrc_list_get_rsrc_type
		_zend_wrong_param_count
		_zend_throw_exception
		_zval_used_for_init
		)
    set(PHP_MODULE_CXX_FLAGS
      "${PHP_MODULE_CXX_FLAGS},-U,${symbol}")
  endforeach()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PHP_INCLUDE_DIRS})
if (MSVC)
	# MSVC does not include inttypes.h but SWIG needs it
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../msvc)
endif()


SET(CMAKE_SWIG_FLAGS "-c++")
if (64BIT_HOST)
	list(APPEND CMAKE_SWIG_FLAGS "-D64BIT_HOST")
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(APPEND CMAKE_SWIG_FLAGS "-DDEBUG")
endif()

SET_SOURCE_FILES_PROPERTIES(umundocore.i PROPERTIES SWIG_FLAGS "-w401,362;")
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/umundocorePHP5_wrap.c PROPERTIES LANGUAGE CXX)
SWIG_ADD_MODULE(umundoNativePHP php5 umundocore.i)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/umundocorePHP5_wrap.c PROPERTIES COMPILE_FLAGS "-Wno-dynamic-class-memaccess")
endif()

# SWIG_LINK_LIBRARIES(umundoNativePHP ${PHP_LIBRARIES})
SWIG_LINK_LIBRARIES(umundoNativePHP umundocore)

if (APPLE)
	set_target_properties(umundoNativePHP PROPERTIES LINK_FLAGS ${PHP_MODULE_CXX_FLAGS})
endif()

if (PHP_ZTS_ENABLED)
	# we are only building php bindings for unices anyhow
	set(PHP_COMPILE_FLAGS "${PHP_COMPILE_FLAGS} -DZTS")
	set(PHP_COMPILE_FLAGS "${PHP_COMPILE_FLAGS} -DPTHREADS")
endif()
if (PHP_DEBUG_ENABLED)
	set(PHP_COMPILE_FLAGS "${PHP_COMPILE_FLAGS} -DZEND_DEBUG")	
endif()

set_target_properties(umundoNativePHP PROPERTIES FOLDER "Bindings")

set(UMUNDO_PHP_WRAPPER "umundoNativePHP${CMAKE_LIBRARY_POSTFIX}.php")
string(REGEX REPLACE "NativePHP" "" UMUNDO_PHP_WRAPPER_DEST ${UMUNDO_PHP_WRAPPER})

ADD_CUSTOM_COMMAND(
	TARGET umundoNativePHP
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy 
		${CMAKE_CURRENT_BINARY_DIR}/${UMUNDO_PHP_WRAPPER}
		${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${UMUNDO_PHP_WRAPPER_DEST}
)

INSTALL_LIBRARY(TARGETS umundoNativePHP COMPONENT librarySwig DESTINATION share/umundo/lib)
