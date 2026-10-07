cmake_minimum_required(VERSION 4.2)
project(aggregator)

set(CMAKE_CXX_STANDARD 23)

add_executable(aggregator
	main.cpp
	arguments.cpp
)
