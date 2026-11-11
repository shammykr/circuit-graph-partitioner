cmake_minimum_required(VERSION 3.16)
project(circuit_graph_partitioner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(CGP_BUILD_TESTS "Build GoogleTest cases" ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(cgp
  src/graph.cpp
  src/hypergraph.cpp
  src/partition.cpp
  src/netlist_io.cpp
  src/kernighan_lin.cpp
  src/fiduccia_mattheyses.cpp
  src/multistart.cpp
)
target_include_directories(cgp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)
target_link_libraries(cgp PUBLIC Threads::Threads)
