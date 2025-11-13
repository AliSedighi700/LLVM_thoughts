 cmake_minimum_required(VERSION 3.10)
project(test LANGUAGES CXX)


find_package(OpenCV REQUIRED)
find_package(benchmark REQUIRED)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable (just main.cpp)
add_executable(test test.cpp)
target_link_libraries(test benchmark::benchmark)
