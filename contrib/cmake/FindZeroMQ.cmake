FIND_PATH(ZeroMQ_INCLUDE_DIR zmq.h
  HINTS $ENV{ZeroMQ_INCLUDE_DIR}
  PATH_SUFFIXES include zeromq
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

set(ZeroMQ_LIBRARY)
FIND_LIBRARY(ZeroMQ_LIBRARY_RELEASE 
  NAMES zmq libzmq
  HINTS $ENV{ZeroMQ_LIBRARY}
  PATH_SUFFIXES lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (ZeroMQ_LIBRARY_RELEASE)
	list(APPEND ZeroMQ_LIBRARY optimized ${ZeroMQ_LIBRARY_RELEASE})
endif()

FIND_LIBRARY(ZeroMQ_LIBRARY_DEBUG
  NAMES libzmq_d zmq_d
  HINTS
  $ENV{ZeroMQ_LIBRARY}
  PATH_SUFFIXES lib
  ${ZEROMQ_PREBUILT}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (ZeroMQ_LIBRARY_DEBUG)
	list(APPEND ZeroMQ_LIBRARY debug ${ZeroMQ_LIBRARY_DEBUG})
else()
	if (NOT WIN32)
		# fall back to release library in debug builds on unices
		list(APPEND ZeroMQ_LIBRARY debug ${ZeroMQ_LIBRARY_RELEASE})
	endif()
endif()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZeroMQ DEFAULT_MSG ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR)
MARK_AS_ADVANCED(ZeroMQ_LIBRARY_RELEASE ZeroMQ_LIBRARY_DEBUG)
