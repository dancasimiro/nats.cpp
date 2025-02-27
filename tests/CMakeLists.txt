cmake_minimum_required(VERSION 3.31)

# Set the project name
project(CPPReplProjectTests)

# Enable testing
enable_testing()

# Add the GoogleTest framework
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add the test executable
add_executable(repl_test repl_test.cpp)

# Link the test executable against the GoogleTest libraries
target_link_libraries(repl_test ${GTEST_LIBRARIES} pthread)

# Add a test
add_test(NAME ReplTest COMMAND repl_test)