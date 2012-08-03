add_executable(test-core-threads test-threads.cpp)
target_link_libraries(test-core-threads ${UMUNDOCORE_LIBRARIES})
set_target_properties(test-core-threads PROPERTIES FOLDER "Tests")

add_test(test-core-threads ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test-core-threads)
add_dependencies(ALL_TESTS test-core-threads)

