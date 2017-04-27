cmake_minimum_required(VERSION 2.8.11)

project(AccumServer)

set (CMAKE_BUILD_TYPE Debug)

add_subdirectory(AccumException)
add_subdirectory(AccumLog)

set (sources	main.cpp
				AccumServer.cpp
)

set (headers	json.hpp
				globals.h
				AccumServer.h
)

add_executable(${CMAKE_PROJECT_NAME} ${sources} ${headers})
target_link_libraries(${CMAKE_PROJECT_NAME} AccumException AccumLog pthread)

