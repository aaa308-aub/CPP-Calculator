set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.20)
project(CLI_Calculator_in_CPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -pedantic -Werror -g -fno-limit-debug-info -fstandalone-debug)

add_executable(calculator main.cpp calculator.h)

add_executable(calculator_test test.cpp calculator.h)