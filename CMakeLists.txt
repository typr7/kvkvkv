cmake_minimum_required(VERSION 3.24)

project(kvkvkv VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(kv_core
  src/memory_kv.cpp
  src/internal_key.cpp
  src/skip_list.cpp
  src/memtable.cpp
)

target_compile_features(kv_core PUBLIC cxx_std_20)

target_include_directories(kv_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_options(kv_core
  PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(CTest)

if(BUILD_TESTING)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(googletest)

  add_executable(kv_tests
    tests/memory_kv_test.cpp
    tests/internal_key_test.cpp
  )

  target_link_libraries(kv_tests
    PRIVATE
      kv_core
      GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(kv_tests)
endif()