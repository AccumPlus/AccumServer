cmake_minimum_required(VERSION 2.8.11)

project(AccumLog)

set (CMAKE_BUILD_TYPE Debug)

set (sources	accumlog.cpp
)

set (headers	accumlog.h
				../globals.h
				../AccumException/accumexception.h
)

add_library(${PROJECT_NAME} SHARED ${sources} ${headers})

target_link_libraries(${PROJECT_NAME} pthread)
