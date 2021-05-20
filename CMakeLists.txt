cmake_minimum_required(VERSION 3.0.0)
project(droprun VERSION 0.1.0)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

set(EXECUTABLE_OUTPUT_PATH "bin")
include_directories(${GTK3_INCLUDE_DIRS})
link_directories($GTK3_LIBRARY_DIRS)
add_definitions(${GTK3_CFLAGS_OTHER})

add_executable(droprun src/main.c)
target_link_libraries(droprun ${GTK3_LIBRARIES})