include(CheckFunctionExists)
include(CheckCXXSourceCompiles)
include(CheckCSourceCompiles)

CHECK_FUNCTION_EXISTS(strndup HAVE_STRNDUP)
if (NOT HAVE_STRNDUP)
	add_definitions("-DNO_STRNDUP")
endif()

CHECK_FUNCTION_EXISTS(strnlen HAVE_STRNLEN)
if (NOT HAVE_STRNLEN)
	add_definitions("-DNO_STRNLEN")
endif()
