cmake_minimum_required(VERSION 3.31)

# Set the project name
project(CPPReplProjectTests)

# Enable testing
enable_testing()

# Add the Catch2 framework
find_package(Catch2 3 REQUIRED)
# These tests can use the Catch2-provided main
add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE natscpp simdjson Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(tests)
