cmake_minimum_required(VERSION 3.16)

project(development_of_information_security_tools_lab_2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allocator is expected to be available locally in the repository.
set(ALLOCATOR_DIR "" CACHE PATH "Path to local Allocator sources")

if(NOT ALLOCATOR_DIR)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Allocator/Allocator.cpp")
		set(ALLOCATOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Allocator")
	elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/allocator/Allocator.cpp")
		set(ALLOCATOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/allocator")
	endif()
endif()

if(NOT EXISTS "${ALLOCATOR_DIR}/Allocator.cpp")
	message(FATAL_ERROR
		"Allocator source was not found. Put Allocator near project modules and set -DALLOCATOR_DIR=/path/to/Allocator"
	)
endif()

add_library(lab2_allocator STATIC
	${ALLOCATOR_DIR}/Allocator.cpp
)

target_include_directories(lab2_allocator PUBLIC
	${ALLOCATOR_DIR}
)

add_subdirectory(BBV)
add_subdirectory(bool_interval)
add_subdirectory(bool_equation)
add_subdirectory(SAT_DPLL)

add_executable(${PROJECT_NAME}
	main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/SAT_DPLL
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	bool_equation
	sat_dpll
)