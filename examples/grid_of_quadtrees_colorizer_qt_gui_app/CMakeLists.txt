string(
    CONCAT
    LAZYBRUSH_BUILD_EXAMPLE_GRID_OF_QUADTREES_QT_GUI_APP_COMMENT
    "Build the example app that uses the grid of quadtrees\n"
    "    - Required dependencies:\n"
    "       - Qt5 Widgets\n"
    "         url: https://www.qt.io/ (not included)"
)

option(
    LAZYBRUSH_BUILD_EXAMPLE_GRID_OF_QUADTREES_QT_GUI_APP
    ${LAZYBRUSH_BUILD_EXAMPLE_GRID_OF_QUADTREES_QT_GUI_APP_COMMENT}
    ON
)

if(LAZYBRUSH_BUILD_EXAMPLE_GRID_OF_QUADTREES_QT_GUI_APP)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)

    add_executable(
        grid_of_quadtrees_colorizer_qt_gui_app
        main.cpp
        window.cpp
        window_ui.cpp
        window.h
        scribble.cpp
        scribble.hpp
        palette.cpp
        ../preprocessing/skeleton_zhang_suen.cpp
        ../preprocessing/skeleton_chen_hsu.cpp
        ../preprocessing/skeleton.hpp
        ../preprocessing/threshold.cpp
        ../preprocessing/threshold.hpp
        ../../third_party/maxflow/graph.cpp
        ../../third_party/maxflow/maxflow.cpp
    )

    target_include_directories(
        grid_of_quadtrees_colorizer_qt_gui_app
        PRIVATE
        ../../include
        ../../third_party/
    )
    target_link_libraries(
        grid_of_quadtrees_colorizer_qt_gui_app
        PRIVATE
        lazybrush
        Qt5::Widgets
    )
    set_target_properties(
        grid_of_quadtrees_colorizer_qt_gui_app
        PROPERTIES
        AUTOMOC ON
        RUNTIME_OUTPUT_DIRECTORY ${LAZYBRUSH_EXAMPLES_OUTPUT_DIRECTORY}
    )
endif()
