cmake_minimum_required(VERSION 3.7)
project(reactive_v2_test)

#set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-std=c++1z")

include_directories(../src)

set(SOURCE_FILES main.cpp)
add_executable(reactive_v2_test ${SOURCE_FILES})