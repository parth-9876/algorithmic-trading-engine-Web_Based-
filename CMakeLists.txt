cmake_minimum_required(VERSION 3.20)
project(AlgoExchange LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only compile main.cpp — it #includes the other .cpp files
add_executable(AlgoExchange main.cpp)
