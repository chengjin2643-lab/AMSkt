cmake_minimum_required(VERSION 3.20)
project(AMSkt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(amskt_fms INTERFACE)
target_include_directories(amskt_fms INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/amskt)
target_compile_features(amskt_fms INTERFACE cxx_std_20)

add_executable(amskt_demo examples/demo.cpp)
target_link_libraries(amskt_demo PRIVATE amskt_fms)

include(CTest)
if(BUILD_TESTING)
  add_executable(amskt_core_tests tests/core_tests.cpp)
  target_link_libraries(amskt_core_tests PRIVATE amskt_fms)
  add_test(NAME core_properties COMMAND amskt_core_tests)

  add_executable(amskt_regression_tests tests/fms_checks.cpp)
  target_include_directories(amskt_regression_tests PRIVATE tests/reference)
  target_link_libraries(amskt_regression_tests PRIVATE amskt_fms)
  add_test(NAME experiment_regression COMMAND amskt_regression_tests)

  add_test(NAME synthetic_demo COMMAND amskt_demo)
endif()

option(AMSKT_BUILD_EXPERIMENTS "Build the Synthetic-10K experiment runner" OFF)
if(AMSKT_BUILD_EXPERIMENTS)
  find_package(Threads REQUIRED)
  add_executable(amskt_compare experiments/unified_fms.cpp)
  target_link_libraries(amskt_compare PRIVATE amskt_fms Threads::Threads)
endif()
