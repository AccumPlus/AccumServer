cmake_minimum_required(VERSION 2.8.11)

project(AccumException)

set (CMAKE_BUILD_TYPE Debug)

set (sources	accumexception.cpp
)

set (headers	accumexception.h
				../globals.h
)

add_library(${PROJECT_NAME} SHARED ${sources} ${headers})
