#
# This file gets called from the host-specific test files in hosts/. They are
# expected to set the following CTest variables:
#
# CTEST_CMAKE_GENERATOR     - the generator to use for cmake (e.g. "Unix Makefiles")
# CTEST_SITE                - the name of this build-slave
# CTEST_BUILD_CONFIGURATION - "Debug", "Release" ..
# CTEST_BUILD_NAME          - name of the particular build (e.g. "darwin-x86_64 llvm bonjour")
#
# CTEST_BUILD_OPTIONS       - parameters to be passed to cmake when preparing the build
#                             these will most likely come from one of the tests/*.ctest files
#
# In addition, we expect the following environment variables to be set:
#
# CTEST_SUBMIT_TYPE - "Experimental", "Nightly", "Continuous"
# UMUNDO_SOURCE_DIR - full path to the umundo sources we will work with
#
# We will always build in /tmp/build-*
#

if ("$ENV{CTEST_SUBMIT_TYPE}" STREQUAL "")
	message(FATAL_ERROR "ENV{CTEST_SUBMIT_TYPE} is emtpy")
endif()

if ("$ENV{UMUNDO_SOURCE_DIR}" STREQUAL "")
	message(FATAL_ERROR "ENV{UMUNDO_SOURCE_DIR} is emtpy")
endif()

set(CTEST_SOURCE_DIRECTORY $ENV{UMUNDO_SOURCE_DIR})
set(CTEST_BINARY_DIRECTORY "/tmp/build-${CTEST_SUBMIT_TYPE}-${CTEST_SCRIPT_NAME}")
set(CTEST_SUBMIT_TYPE $ENV{CTEST_SUBMIT_TYPE})

set(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE 1048576)
set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 1048576)

### Coverage and Memory checks? (untested) ################################

#set(WITH_COVERAGE TRUE)
#find_program(CTEST_COVERAGE_COMMAND NAMES gcov)

#set(WITH_MEMCHECK TRUE)
#find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
#set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE ${CTEST_SOURCE_DIRECTORY}/tests/valgrind.supp)

### Versioning Control ###################################################

find_program(CTEST_GIT_COMMAND NAMES git)
if(NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
  set(CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} clone git://github.com/tklab-tud/umundo.git ${CTEST_SOURCE_DIRECTORY}")
endif()

set(CTEST_UPDATE_COMMAND "${CTEST_GIT_COMMAND}")

### CMake Command line ###################################################

set(CTEST_CONFIGURE_COMMAND "${CMAKE_COMMAND} -LA -DCMAKE_BUILD_TYPE:STRING=${CTEST_BUILD_CONFIGURATION}")
set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} \"-G${CTEST_CMAKE_GENERATOR}\"")

if (CTEST_TOOLCHAIN)
	set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} -DCMAKE_TOOLCHAIN_FILE=${CTEST_TOOLCHAIN} ${CTEST_BUILD_OPTIONS}")
else()
	set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} -DBUILD_TESTS=ON ${CTEST_BUILD_OPTIONS}")
endif()

set(CTEST_CONFIGURE_COMMAND "${CTEST_CONFIGURE_COMMAND} \"${CTEST_SOURCE_DIRECTORY}\"")

#message(FATAL_ERROR "CTEST_CONFIGURE_COMMAND: ${CTEST_CONFIGURE_COMMAND}")

### Test! ################################################################

# clean build directory
ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})

ctest_start(${CTEST_SUBMIT_TYPE}) # start testing
ctest_update()                    # synchronize with git head
ctest_configure()                 # run cmake
ctest_build()                     # run build-system
ctest_test()                      # run tests

if (WITH_MEMCHECK AND CTEST_COVERAGE_COMMAND)
  ctest_coverage()
endif (WITH_MEMCHECK AND CTEST_COVERAGE_COMMAND)
if (WITH_MEMCHECK AND CTEST_MEMORYCHECK_COMMAND)
  ctest_memcheck()
endif (WITH_MEMCHECK AND CTEST_MEMORYCHECK_COMMAND)

ctest_submit()                    # submit
