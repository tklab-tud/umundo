FIND_PATH(BonjourEmbedded_INCLUDE_DIR mDNSEmbeddedAPI.h
  HINTS
  $ENV{BonjourEmbeddedDIR}
  PATH_SUFFIXES bonjour
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

set(BonjourEmbedded_LIBRARY)

FIND_LIBRARY(BonjourEmbedded_LIBRARY_RELEASE
  NAMES mDNSEmbedded
  HINTS
  $ENV{BonjourEmbeddedDIR}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

if (BonjourEmbedded_LIBRARY_RELEASE)
	list(APPEND BonjourEmbedded_LIBRARY optimized ${BonjourEmbedded_LIBRARY_RELEASE})
endif()

FIND_LIBRARY(BonjourEmbedded_LIBRARY_DEBUG
  NAMES mDNSEmbedded_d mDNSEmbedded
  HINTS
  $ENV{BonjourEmbeddedDIR}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)
if (BonjourEmbedded_LIBRARY_DEBUG)
	list(APPEND BonjourEmbedded_LIBRARY debug ${BonjourEmbedded_LIBRARY_DEBUG})
endif()

# handle the QUIETLY and REQUIRED arguments and set OPENAL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BonjourEmbedded DEFAULT_MSG BonjourEmbedded_LIBRARY BonjourEmbedded_INCLUDE_DIR)
MARK_AS_ADVANCED(BonjourEmbedded_LIBRARY_RELEASE BonjourEmbedded_LIBRARY_DEBUG)
