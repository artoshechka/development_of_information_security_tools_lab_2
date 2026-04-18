add_library(bool_equation STATIC
    src/boolequation.cpp
    src/min_dont_care_branching_strategy.cpp
    src/first_free_column_branching_strategy.cpp
    src/branching_strategy_factory.cpp
)

target_include_directories(bool_equation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bool_equation PUBLIC
    bool_interval
)
