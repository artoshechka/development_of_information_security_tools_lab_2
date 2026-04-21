cmake_minimum_required(VERSION 3.16)

add_executable(laba_SAT1
    ../main.cpp
    src/nodebooltree.cpp
)

target_include_directories(laba_SAT1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(laba_SAT1 PRIVATE
    bool_equation
)