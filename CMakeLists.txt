cmake_minimum_required(VERSION 4.0.2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CPP-Calculator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Werror -g)

add_executable(calculator main.cpp)

add_executable(calculator_test test.cpp)
