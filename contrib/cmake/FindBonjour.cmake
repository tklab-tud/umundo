FIND_PATH(Bonjour_INCLUDE_DIR dns_sd.h
  HINTS
  $ENV{BonjourDIR}
  PATH_SUFFIXES include bonjour
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

set(Bonjour_LIBRARY)

FIND_LIBRARY(Bonjour_LIBRARY_RELEASE
  NAMES dnssd dns_sd
  HINTS
  $ENV{BonjourDIR}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (Bonjour_LIBRARY_RELEASE)
	list(APPEND Bonjour_LIBRARY optimized ${Bonjour_LIBRARY_RELEASE})
endif()

FIND_LIBRARY(Bonjour_LIBRARY_DEBUG
  NAMES dnssd dns_sd_d dnssd dns_sd
  HINTS
  $ENV{BonjourDIR}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (Bonjour_LIBRARY_DEBUG)
	list(APPEND Bonjour_LIBRARY debug ${Bonjour_LIBRARY_DEBUG})
endif()

# handle the QUIETLY and REQUIRED arguments and set OPENAL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Bonjour DEFAULT_MSG Bonjour_LIBRARY Bonjour_INCLUDE_DIR)
MARK_AS_ADVANCED(Bonjour_LIBRARY_RELEASE Bonjour_LIBRARY_DEBUG)
