if (APPLE)
	# wont compile due to @autoreleasepool clang extension
	#
	# include_directories(${PROJECT_SOURCE_DIR}/contrib/bindings/objc)
	# add_executable(s11n-test-api-objc 
	# 	test-api.mm
	# 	${PROJECT_SOURCE_DIR}/contrib/bindings/objc/umundo-objc/core/UMNode.mm
	# 	${PROJECT_SOURCE_DIR}/contrib/bindings/objc/umundo-objc/core/UMPublisher.mm
	# 	${PROJECT_SOURCE_DIR}/contrib/bindings/objc/umundo-objc/core/UMSubscriber.mm
	# 	${PROJECT_SOURCE_DIR}/contrib/bindings/objc/umundo-objc/s11n/UMTypedSubscriber.mm
	# 	${PROJECT_SOURCE_DIR}/contrib/bindings/objc/umundo-objc/s11n/UMTypedPublisher.mm
	# )
	# FIND_LIBRARY(FOUNDATION NAMES FOUNDATION)
	# target_link_libraries(s11n-test-api-objc 
	# 	${FOUNDATION} 
	# 	${UMUNDOCORE_LIBRARIES} 
	# 	${UMUNDOS11N_LIBRARIES})
endif()

# add_executable(test-s11n-api test-api.cpp)
# target_link_libraries(test-s11n-api ${UMUNDOCORE_LIBRARIES})
# target_link_libraries(test-s11n-api ${UMUNDOS11N_LIBRARIES})
# set_target_properties(test-s11n-api PROPERTIES FOLDER "Tests")
