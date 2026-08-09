cmake_minimum_required(VERSION 3.10)

# Define your project name and programming language
project(MySimpleProject LANGUAGES CXX)

# Enforce the modern C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create an executable program named "my_program" using main.cpp
add_executable(my_program main.cpp)
target_link_libraries(my_program PUBLIC uv)