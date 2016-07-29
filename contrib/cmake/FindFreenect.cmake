set(FREENECT_INCLUDE_DIR)
FIND_PATH(FREENECT_HPP_INCLUDE_DIR libfreenect.hpp
  HINTS $ENV{FREENECT_INCLUDE_DIR}
  PATH_SUFFIXES include
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)
if (FREENECT_HPP_INCLUDE_DIR)
	list(APPEND FREENECT_INCLUDE_DIR ${FREENECT_HPP_INCLUDE_DIR})
endif()
FIND_PATH(FREENECT_H_INCLUDE_DIR libfreenect.h
  PATH_SUFFIXES include include/libfreenect
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)
if (FREENECT_H_INCLUDE_DIR)
	list(APPEND FREENECT_INCLUDE_DIR ${FREENECT_H_INCLUDE_DIR})
endif()

if (NOT WIN32)
	pkg_check_modules (LIBUSB_PKG libusb-1.0)
	find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h
	  PATHS
	  ${LIBUSB_PKG_INCLUDE_DIRS}
	  /usr/include/libusb-1.0
	  /usr/include
	  /usr/local/include
	)
endif()

if (LIBUSB_INCLUDE_DIR)
	list(APPEND FREENECT_INCLUDE_DIR ${LIBUSB_INCLUDE_DIR})
endif()

set(FREENECT_LIBRARY)
FIND_LIBRARY(FREENECT_LIBRARY_RELEASE
  NAMES freenect
  HINTS $ENV{FREENECT_LIBRARY}
  PATH_SUFFIXES lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (FREENECT_LIBRARY_RELEASE)
	list(APPEND FREENECT_LIBRARY optimized ${FREENECT_LIBRARY_RELEASE})
endif()

FIND_LIBRARY(FREENECT_LIBRARY_DEBUG
  NAMES freenect_d
  HINTS $ENV{FREENECT_LIBRARY}
  PATH_SUFFIXES lib
  ${FREENECT_PREBUILT}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

if (FREENECT_LIBRARY_DEBUG)
	list(APPEND FREENECT_LIBRARY debug ${FREENECT_LIBRARY_DEBUG})
else()
	if (NOT WIN32)
		# fall back to release library in debug builds on unices
		list(APPEND FREENECT_LIBRARY debug ${FREENECT_LIBRARY_RELEASE})
	endif()
endif()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FREENECT DEFAULT_MSG FREENECT_LIBRARY FREENECT_INCLUDE_DIR)
MARK_AS_ADVANCED(FREENECT_LIBRARY_RELEASE FREENECT_LIBRARY_DEBUG)
