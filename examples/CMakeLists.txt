option(LAZYBRUSH_BUILD_EXAMPLES "Build the examples" OFF)

if(LAZYBRUSH_BUILD_EXAMPLES)
    set(
        LAZYBRUSH_EXAMPLES_OUTPUT_DIRECTORY
        "${LAZYBRUSH_OUTPUT_DIRECTORY}/examples"
    )

    add_subdirectory(grid_of_quadtrees_colorizer_qt_gui_app)

endif()
