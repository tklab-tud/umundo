add_executable(test-core-discovery test-discovery.cpp)
target_link_libraries(test-core-discovery ${UMUNDOCORE_LIBRARIES})
add_test(test-core-discovery ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-core-discovery)
set_target_properties(test-core-discovery PROPERTIES FOLDER "Tests")
add_dependencies(ALL_TESTS test-core-discovery)

add_executable(test-core-domains test-domains.cpp)
target_link_libraries(test-core-domains ${UMUNDOCORE_LIBRARIES})
add_test(test-core-domains ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-core-domains)
set_target_properties(test-core-domains PROPERTIES FOLDER "Tests")
add_dependencies(ALL_TESTS test-core-domains)

