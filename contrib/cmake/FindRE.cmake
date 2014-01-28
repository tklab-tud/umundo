FIND_PATH(RE_INCLUDE_DIR re.h
  HINTS $ENV{RE_INCLUDE_DIR}
  PATH_SUFFIXES re include include/re
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

set(RE_LIBRARY)
FIND_LIBRARY(RE_LIBRARY_RELEASE
  NAMES re
  HINTS $ENV{RE_LIBRARY}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (RE_LIBRARY_RELEASE)
	list(APPEND RE_LIBRARY optimized ${RE_LIBRARY_RELEASE})
endif()

FIND_LIBRARY(RE_LIBRARY_DEBUG
  NAMES re_d
  HINTS
  $ENV{REDIR}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (RE_LIBRARY_DEBUG)
	list(APPEND RE_LIBRARY debug ${RE_LIBRARY_DEBUG})
else()
	if (NOT WIN32)
		# fall back to release library in debug builds on unices
		list(APPEND RE_LIBRARY debug ${RE_LIBRARY_RELEASE})
	endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set OPENAL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RE  DEFAULT_MSG  RE_LIBRARY RE_INCLUDE_DIR)
MARK_AS_ADVANCED(RE_LIBRARY_RELEASE RE_LIBRARY_DEBUG)
