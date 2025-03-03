cmake_minimum_required(VERSION 3.31)

project(nats.cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O0 -coverage")

# Add the Boost ASIO library.
find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(include)

add_library(simdjson STATIC src/simdjson.cpp)
add_library(natscpp STATIC src/nats_client.cpp)

add_executable(repl src/repl.cpp src/main.cpp)
target_link_libraries(repl natscpp simdjson ${Boost_LIBRARIES})

# Enable testing
enable_testing()

# Add the Catch2 framework
find_package(Catch2 3 REQUIRED)
# These tests can use the Catch2-provided main
add_executable(tests tests/test.cpp)
target_link_libraries(tests PRIVATE natscpp simdjson Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(tests)


# Generate Coverage Report
find_program(LCOV lcov REQUIRED)
find_program(GENHTML genhtml REQUIRED)

# add coverage target
add_custom_target(coverage
  # gather data
  COMMAND ${LCOV} --directory . --include nats_client.*pp --capture --output-file lcov.info
  # generate report
  COMMAND ${GENHTML} --demangle-cpp -o coverage lcov.info
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
