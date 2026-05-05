cmake_minimum_required(VERSION 3.16)

add_library(sat_dpll STATIC
    src/nodebooltree.cpp
)

target_include_directories(sat_dpll PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../BBV
    ${CMAKE_CURRENT_SOURCE_DIR}/../bool_interval
    ${CMAKE_CURRENT_SOURCE_DIR}/../bool_equation
)

target_link_libraries(sat_dpll PUBLIC
	lab2_allocator
)