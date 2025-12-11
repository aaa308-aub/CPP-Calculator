cmake_minimum_required(VERSION 4.0.2)

project(CPP_Calculator)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(calculator
        src/calculator.cpp
        src/main.cpp
)

add_executable(calculator_test
        src/calculator.cpp
        src/test.cpp
)

target_include_directories(calculator PRIVATE src)
target_include_directories(calculator_test PRIVATE src)

target_compile_options(calculator PRIVATE -O2)
target_compile_options(calculator_test PRIVATE -O2)