add_library(bool_interval STATIC
    src/boolinterval.cpp
)

target_include_directories(bool_interval PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bool_interval PUBLIC
    lab2_allocator
    BBV
)
