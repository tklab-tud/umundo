FIND_PATH(Avahi_INCLUDE_DIR avahi-client/client.h
  HINTS
  $ENV{AvahiDIR}
  PATH_SUFFIXES include
  PATHS
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(Avahi_Client_LIBRARY 
  NAMES avahi-client
  HINTS
  $ENV{AvahiDIR}
  ${AVAHI_PREBUILT}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

FIND_LIBRARY(Avahi_Common_LIBRARY 
  NAMES avahi-common
  HINTS
  $ENV{AvahiDIR}
  ${AVAHI_PREBUILT}
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

FIND_LIBRARY(DBus_LIBRARY 
  NAMES dbus-1
  HINTS
  $ENV{DBusDIR}
  PATHS
  /usr/local
  /usr
	/usr/lib64
  /sw
  /opt/local
  /opt/csw
  /opt
)

SET(Avahi_LIBRARIES ${Avahi_Client_LIBRARY})
LIST(APPEND Avahi_LIBRARIES ${Avahi_Common_LIBRARY})
LIST(APPEND Avahi_LIBRARIES ${DBus_LIBRARY})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Avahi DEFAULT_MSG Avahi_Client_LIBRARY Avahi_Common_LIBRARY Avahi_INCLUDE_DIR DBus_LIBRARY)
MARK_AS_ADVANCED(Avahi_INCLUDE_DIR Avahi_Client_LIBRARY Avahi_Common_LIBRARY DBus_LIBRARY)
