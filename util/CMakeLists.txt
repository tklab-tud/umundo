############################################################
# General setup
############################################################

# gather all header files
file(GLOB_RECURSE UMUNDOUTIL_HEADER_FILES src/*.h)

file(GLOB_RECURSE UTIL_FILES src/umundo/util/*.cpp)
list(APPEND UMUNDOUTIL_FILES ${UTIL_FILES})

# include directories for our header files
include_directories(src)

LIST(APPEND UMUNDOUTIL_LIBRARIES umundoutil)
LIST(SORT UMUNDOUTIL_HEADER_FILES)

add_library(umundoutil ${UMUNDOUTIL_FILES})
target_link_libraries(umundoutil ${UMUNDOUTIL_LIBRARIES})
INSTALL_HEADERS(HEADERS ${UMUNDOUTIL_HEADER_FILES} COMPONENT headers)
INSTALL_LIBRARY(TARGETS umundoutil COMPONENT libraryUtil)
set_target_properties(umundoutil PROPERTIES FOLDER "Libraries")

# ok to have in IDEs, but do not install
LIST(APPEND UMUNDOUTIL_HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/../umundo/config.h)

############################################################
# Miscellaneous
############################################################

set(UMUNDOUTIL_FILES "${UMUNDOUTIL_FILES}" PARENT_SCOPE)
set(UMUNDOUTIL_LIBRARIES "${UMUNDOUTIL_LIBRARIES}" PARENT_SCOPE)
set(UMUNDOUTIL_HEADER_FILES "${UMUNDOUTIL_HEADER_FILES}" PARENT_SCOPE)

