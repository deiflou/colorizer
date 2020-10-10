cmake_minimum_required(VERSION 3.1.0)

project(lazybrush VERSION 0.1.0)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(LAZYBRUSH_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build")
if(WIN32)
    set(LAZYBRUSH_WIN32_EXECUTABLE WIN32)
endif()

add_library(lazybrush INTERFACE)
target_include_directories(
    lazybrush
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
add_library(lazybrush::lazybrush ALIAS lazybrush)

install(TARGETS lazybrush EXPORT lazybrushConfig)
install(EXPORT lazybrushConfig DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/lazybrush")
install(DIRECTORY include/lazybrush DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(examples)
