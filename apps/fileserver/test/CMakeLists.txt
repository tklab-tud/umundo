set_source_files_properties(
	${CMAKE_BINARY_DIR}/protobuf/generated/DirectoryListingService.pb.cc 
	${CMAKE_BINARY_DIR}/protobuf/generated/DirectoryListingService.rpc.pb.cc
PROPERTIES GENERATED TRUE)

add_executable(test-util-directory-monitoring test-directory-monitoring.cpp
	"../umundo/filesystem/DirectoryListingClient.cpp"
	"../umundo/filesystem/DirectoryListingService.cpp"
	"${CMAKE_BINARY_DIR}/protobuf/generated/DirectoryListingService.pb.cc"
	"${CMAKE_BINARY_DIR}/protobuf/generated/DirectoryListingService.rpc.pb.cc"
)
target_link_libraries(test-util-directory-monitoring ${UMUNDOCORE_LIBRARIES})
target_link_libraries(test-util-directory-monitoring ${UMUNDOUTIL_LIBRARIES})
target_link_libraries(test-util-directory-monitoring ${UMUNDORPC_LIBRARIES})
target_link_libraries(test-util-directory-monitoring ${UMUNDOS11N_LIBRARIES})
set_target_properties(test-util-directory-monitoring PROPERTIES FOLDER "Tests")

add_executable(test-util-structured-cache test-structured-cache.cpp
	"../umundo/cache/RandomCache.cpp"
	"../umundo/cache/StructuredCache.cpp"
	"../umundo/cache/NBandCache.cpp"
	"../umundo/cache/CacheProfiler.cpp"
	"../umundo/prediction/SPA.cpp"
)
target_link_libraries(test-util-structured-cache ${UMUNDOCORE_LIBRARIES})
set_target_properties(test-util-structured-cache PROPERTIES FOLDER "Tests")

add_executable(test-sequence-prediction test-sequence-prediction.cpp
	"../umundo/cache/StructuredCache.cpp"
	"../umundo/cache/NBandCache.cpp"
	"../umundo/cache/CacheProfiler.cpp"
	"../umundo/prediction/SPA.cpp"
	"../umundo/prediction/AFxL.cpp"
)
target_link_libraries(test-sequence-prediction ${UMUNDOCORE_LIBRARIES})
set_target_properties(test-sequence-prediction PROPERTIES FOLDER "Tests")
