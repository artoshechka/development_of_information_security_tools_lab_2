add_library(BBV STATIC
    src/BBV.cpp
)

target_include_directories(BBV PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BBV PUBLIC
    lab2_allocator
)
