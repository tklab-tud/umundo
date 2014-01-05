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

FIND_LIBRARY(JRTPLIB_LIBRARY
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

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JRTPLIB DEFAULT_MSG JRTPLIB_LIBRARY JRTPLIB_INCLUDE_DIR)
