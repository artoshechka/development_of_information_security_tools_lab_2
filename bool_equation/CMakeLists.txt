add_library(bool_equation STATIC
    src/boolequation.cpp
)

target_include_directories(bool_equation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bool_equation PUBLIC
    bool_interval
)
