cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# ----------------------------------------------------------------------------------------

project(LockFreeQueueTest)

add_definitions("-std=c++11 -Wall -g -pthread")

#set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -v -E")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------------------

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Specify RRD_PATH environment variable with path to the Relacy Race Detector library:
# http://www.1024cores.net/home/relacy-race-detector
include_directories($ENV{RRD_PATH} relacy)

list(APPEND CMAKE_CXX_FLAGS "-pthread")

file(GLOB tests "test/*_test.cpp")

foreach(test_file ${tests})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file})
endforeach()
