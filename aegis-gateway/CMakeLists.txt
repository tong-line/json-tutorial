cmake_minimum_required(VERSION 3.20)
project(aegis_gateway VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AEGIS_WERROR "Treat warnings as errors" OFF)

find_package(Boost 1.74 REQUIRED COMPONENTS system)
find_package(GTest REQUIRED)

add_library(aegis_core
    src/core/version.cpp
)
target_include_directories(aegis_core PUBLIC include)
target_link_libraries(aegis_core PUBLIC Boost::system)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aegis_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
if (AEGIS_WERROR)
    target_compile_options(aegis_core PRIVATE -Werror)
endif()

add_executable(aegis src/main.cpp)
target_link_libraries(aegis PRIVATE aegis_core)

enable_testing()
add_executable(aegis_tests tests/unit/test_smoke.cpp)
target_link_libraries(aegis_tests PRIVATE aegis_core GTest::gtest_main)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(aegis_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GoogleTest)
gtest_discover_tests(aegis_tests)
