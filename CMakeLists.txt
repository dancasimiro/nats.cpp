cmake_minimum_required(VERSION 3.31)

project(nats.cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the Boost ASIO library.
find_package(Boost REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(repl ${SOURCES})
target_link_libraries(repl ${Boost_LIBRARIES})

enable_testing()
add_subdirectory(tests)