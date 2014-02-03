############################################################
# General setup
############################################################

file(GLOB_RECURSE SERVER_FILES umundo/filesystem/*.cpp)
file(GLOB_RECURSE CLIENT_FILES umundo/prediction/*.cpp umundo/cache/*.cpp umundo/filesystem/*.cpp)

if (WIN32)
	list(APPEND SERVER_FILES ${PROJECT_SOURCE_DIR}/contrib/snippets/XGetopt.cpp)
	list(APPEND CLIENT_FILES ${PROJECT_SOURCE_DIR}/contrib/snippets/XGetopt.cpp)
endif()

# include directories for our header files
include_directories(.)

LIST(APPEND SERVER_LIBRARIES umundoutil)
LIST(APPEND SERVER_LIBRARIES umundocore)
LIST(APPEND SERVER_LIBRARIES umundoserial)
LIST(APPEND SERVER_LIBRARIES umundorpc)

LIST(APPEND CLIENT_LIBRARIES umundoutil)
LIST(APPEND CLIENT_LIBRARIES umundocore)
LIST(APPEND CLIENT_LIBRARIES umundoserial)
LIST(APPEND CLIENT_LIBRARIES umundorpc)

############################################################
# run protobuf for umundo
############################################################

include(UseUMUNDO)

# process .proto files
file(GLOB_RECURSE PROTOBUF_INTERFACES *.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

UMUNDO_PROTOBUF_GENERATE_CPP_RPC(PROTO_SRCS PROTO_HDRS ${PROTOBUF_INTERFACES})
list(APPEND SERVER_FILES ${PROTO_SRCS})	
list(APPEND CLIENT_FILES ${PROTO_SRCS})	

############################################################
# create executables
############################################################

add_executable(umundo-fileserver umundo-fileserver.cpp ${SERVER_FILES})
target_link_libraries(umundo-fileserver ${SERVER_LIBRARIES})
set_target_properties(umundo-fileserver PROPERTIES FOLDER "Apps")

add_executable(umundo-fileclient umundo-fileclient.cpp ${CLIENT_FILES})
target_link_libraries(umundo-fileclient ${CLIENT_LIBRARIES})
set_target_properties(umundo-fileclient PROPERTIES FOLDER "Apps")

add_subdirectory(test)