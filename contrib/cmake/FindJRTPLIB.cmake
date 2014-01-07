FIND_PATH(JRTPLIB_INCLUDE_DIR jrtplib3/rtpsession.h
  HINTS $ENV{JRTPLIB_INCLUDE_DIR}
  PATH_SUFFIXES include
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

set(JRTPLIB_LIBRARY)
FIND_LIBRARY(JRTPLIB_LIBRARY_RELEASE
  NAMES jrtp libjrtp
  HINTS $ENV{JRTPLIB_LIBRARY}
  PATH_SUFFIXES lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (JRTPLIB_LIBRARY_RELEASE)
	list(APPEND JRTPLIB_LIBRARY optimized ${JRTPLIB_LIBRARY_RELEASE})
endif()

FIND_LIBRARY(JRTPLIB_LIBRARY_DEBUG
  NAMES jrtp_d libjrtp_d
  HINTS $ENV{JRTPLIB_LIBRARY}
  PATH_SUFFIXES lib
  ${JRTPLIB_PREBUILT}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

if (JRTPLIB_LIBRARY_DEBUG)
	list(APPEND JRTPLIB_LIBRARY debug ${JRTPLIB_LIBRARY_DEBUG})
else()
	if (NOT WIN32)
		# fall back to release library in debug builds on unices
		list(APPEND JRTPLIB_LIBRARY debug ${JRTPLIB_LIBRARY_RELEASE})
	endif()
endif()

message(STATUS "INCLUDE: ${JRTPLIB_INCLUDE_DIR}")
message(STATUS "LIB: ${JRTPLIB_LIBRARY}")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JRTPLIB DEFAULT_MSG JRTPLIB_LIBRARY JRTPLIB_INCLUDE_DIR)
MARK_AS_ADVANCED(JRTPLIB_LIBRARY_RELEASE JRTPLIB_LIBRARY_DEBUG)

message(STATUS "JRTPLIB FOUND: ${JRTPLIB_FOUND}")

