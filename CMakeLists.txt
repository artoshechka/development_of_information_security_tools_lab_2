cmake_minimum_required(VERSION 3.16)

project(development_of_information_security_tools_lab_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Auto-download Allocator from upstream repository during CMake configure.
FetchContent_Declare(
	allocator_upstream
	GIT_REPOSITORY https://github.com/endurodave/Allocator.git
	GIT_TAG master
)

FetchContent_GetProperties(allocator_upstream)
if(NOT allocator_upstream_POPULATED)
	FetchContent_Populate(allocator_upstream)
endif()

add_library(lab2_allocator STATIC
	${allocator_upstream_SOURCE_DIR}/Allocator.cpp
)

target_include_directories(lab2_allocator PUBLIC
	${allocator_upstream_SOURCE_DIR}
)

add_subdirectory(BBV)
add_subdirectory(bool_interval)
add_subdirectory(bool_equation)
add_subdirectory(SAT_DPLL)