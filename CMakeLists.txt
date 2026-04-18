cmake_minimum_required(VERSION 3.16)

project(development_of_information_security_tools_lab_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(BBV)
add_subdirectory(bool_interval)
add_subdirectory(bool_equation)
add_subdirectory(SAT_DPLL)