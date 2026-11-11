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
  src/profiling.cpp
  src/driver.cpp
)
target_include_directories(cgp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)
target_link_libraries(cgp PUBLIC Threads::Threads)

add_executable(cgp_cli src/main.cpp)
set_target_properties(cgp_cli PROPERTIES OUTPUT_NAME cgp)
target_link_libraries(cgp_cli PRIVATE cgp)

add_executable(cgp_gen benchmarks/gen_netlist.cpp)
target_link_libraries(cgp_gen PRIVATE cgp)

add_executable(cgp_bench benchmarks/bench_main.cpp)
target_link_libraries(cgp_bench PRIVATE cgp)

if(CGP_BUILD_TESTS)
  enable_testing()
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
  add_executable(cgp_tests tests/test_partitioners.cpp)
  target_link_libraries(cgp_tests PRIVATE cgp GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(cgp_tests)
endif()
