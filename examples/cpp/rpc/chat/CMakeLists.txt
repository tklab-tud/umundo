cmake_minimum_required(VERSION 2.8)
project(chat-rpc)

#
# We need to set CMAKE_MODULE_PATH to the path where FindUMundo.cmake can be 
# found in order for CMake to be able to search for umundo.
#
find_path(FINDUMUNDO_CMAKE_PATH FindUMundo.cmake
  PATHS
  ${PROJECT_SOURCE_DIR}/../../../../cmake               # we still are in the source distribution
  /usr/local/share/umundo/cmake                         # installed on unices
  "C:/Program Files (x86)/uMundo/share/umundo/cmake"    # installed on Windows
  "C:/Program Files/uMundo/share/umundo/cmake"          # installed on Windows alternative
  ENV UMUNDO_CMAKE_PATH                                 # path provided per ENV variable
)

if (NOT FINDUMUNDO_CMAKE_PATH)
  message(FATAL_ERROR "Could not find directory containing FindUMundo.cmake, set UMUNDO_CMAKE_PATH environment variable to help")
endif()

# add path with FindUMundo.cmake to cmakes module path
set(CMAKE_MODULE_PATH ${FINDUMUNDO_CMAKE_PATH})

find_package(UMUNDO REQUIRED core serial rpc)
include_directories(${UMUNDO_INCLUDE_DIR})

find_package(ProtoBuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${PROTOBUF_INCLUDE_DIRS})

# we do need the boost headers in our public interface
# do we want to install the headers?
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# process .proto files and generate serializer classes
include("${FINDUMUNDO_CMAKE_PATH}/UseUMundo.cmake")
UMUNDO_PROTOBUF_GENERATE_CPP_RPC(PROTO_SRCS PROTO_HDRS "${PROJECT_SOURCE_DIR}/proto/ChatS11N.proto")
include_directories(${CMAKE_BINARY_DIR}/protobuf/generated)

add_executable(chat-rpc 
  src/Chat.cpp
	${PROTO_SRCS}
)
target_link_libraries(chat-rpc 
  ${UMUNDO_LIBRARIES} 
)