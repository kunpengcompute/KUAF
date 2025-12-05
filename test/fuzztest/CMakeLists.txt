cmake_minimum_required(VERSION 3.10)

# Set project name and language
project(kuaf_fuzztest VERSION 1.0 LANGUAGES CXX)

# Set CMake policy to avoid warnings about old policies
# (This is optional but recommended for new projects)
cmake_policy(SET CMP0091 NEW)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define directories
set(KUAF_modules_DIR ${CMAKE_SOURCE_DIR}/../..)
set(TEST_modules_DIR ${CMAKE_SOURCE_DIR}/../)
set(ASAN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(GTEST_INSTALL_LIB_DIR ${TEST_modules_DIR}test_tool_bins/gtest)
SET(LIB_3RD_GMOCK ${GTEST_INSTALL_LIB_DIR}/libgmock.a)
SET(LIB_3RD_GTEST ${GTEST_INSTALL_LIB_DIR}/libgtest.a)

# Header file paths
include_directories(
    ${GTEST_DIR}/include
    ${GMOCK_DIR}/include
    /usr/local/kuaf/include
    ${KUAF_modules_DIR}/../secodefuzz/Secodefuzz/
    ${TEST_modules_DIR}/googletest/googletest/include
)

# Libraries directories to link
link_directories(
    /usr/local/kuaf/lib/
    /usr/local/lib/
    ${ASAN_DIR}
    ${TEST_modules_DIR}/test_tool_bins/gtest
    ${KUAF_modules_DIR}/../secodefuzz/examples/out-bin-x64
)

# Source files
set(SRC_FILES_DIRS ${TEST_modules_DIR}/fuzztest)
file(GLOB SRC_KUAF "${SRC_FILES_DIRS}/kuaf_fuzz.cpp")

# Compile definitions
# if(NOT "${fuzzcount}" STREQUAL "")
#     add_definitions(-DTEST_COUNT=${fuzzcount})
# endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -fPIC -fno-omit-frame-pointer -O0  -fsanitize=address -fsanitize-coverage=trace-pc -fprofile-arcs -ftest-coverage ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall  -fPIC  -fsanitize=address -fno-omit-frame-pointer -O0 -fprofile-arcs -ftest-coverage")

# Fuzz test executable
link_directories(${ASAN_DIR})
add_executable(kuaf_fuzz ${SRC_KUAF})

# Link libraries
#Link libraries : kuaf_fuzz
target_link_libraries(kuaf_fuzz
                    PRIVATE
                    ${LIB_3RD_GMOCK}
                    ${LIB_3RD_GTEST}
                    Secodefuzz
                    kuaf
                    )

# Custom target for fuzz testing
add_custom_target(fuzztest
    COMMAND env LD_LIBRARY_PATH=/usr/local/kuaf/lib/:${ASAN_DIR}/
    COMMAND echo ${func}
    COMMAND ./kuaf_fuzz
    DEPENDS   kuaf_fuzz
)

# Custom target for running the fuzz test script
add_custom_target(run
    COMMAND sh ../ln-s.sh
    COMMAND make fuzztest
    COMMAND python ../../kp_coverage.py
    COMMENT "Running fuzz test and generating HTML report"
    VERBATIM
)