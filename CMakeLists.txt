cmake_minimum_required(VERSION 3.1.0)

project(colorizer)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(
    colorizer
    main.cpp
    window.cpp
    window_ui.cpp
    window.h
    scribble.cpp
    scribble.hpp
    palette.cpp
    preprocessing/skeletonZhangSuen.cpp
    preprocessing/skeletonChenHsu.cpp
    preprocessing/skeleton.hpp
    preprocessing/threshold.cpp
    preprocessing/threshold.hpp
    colorizer/maxflow/graph.cpp
    colorizer/maxflow/maxflow.cpp
)

target_include_directories(colorizer PRIVATE space_partitioning colorizer)

target_link_libraries(colorizer PRIVATE Qt5::Widgets)

set_target_properties(
    colorizer
    PROPERTIES
    AUTOMOC ON
)
