cmake_minimum_required(VERSION 3.20)
project(hft_lob_engine VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Optional optimizations for release builds
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Include FetchContent for dependencies
include(FetchContent)

# Try to find system GoogleTest first
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GoogleTest not found, will use FetchContent if network available")
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# nlohmann/json - try system first
find_package(nlohmann_json 3.11.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found, will use FetchContent if network available")
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(json)
endif()

# Enable testing
enable_testing()

# Core library
add_library(lob_engine STATIC
    src/order_book.cpp
    src/reference_order_book.cpp
    src/matching_engine.cpp
    src/event_log.cpp
    src/telemetry.cpp
    src/market_data.cpp
    src/binary_serializer.cpp
    src/trade_tape.cpp
    src/engine_validator.cpp
    src/market_data_publisher.cpp
)

target_include_directories(lob_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lob_engine PUBLIC
    nlohmann_json::nlohmann_json
)

# Examples
add_executable(simple_demo examples/simple_demo.cpp)
target_link_libraries(simple_demo PRIVATE lob_engine)

add_executable(replay examples/replay.cpp)
target_link_libraries(replay PRIVATE lob_engine)

add_executable(csv_replay examples/csv_replay.cpp)
target_link_libraries(csv_replay PRIVATE lob_engine)

add_executable(live_feed_example examples/live_feed_example.cpp)
target_link_libraries(live_feed_example PRIVATE lob_engine)

# Tests
add_executable(run_tests
    tests/test_order_book_basic.cpp
    tests/test_order_book_partial_fills.cpp
    tests/test_order_book_advanced.cpp
    tests/test_order_management.cpp
    tests/test_self_trade_prevention.cpp
    tests/test_matching_engine.cpp
    tests/test_market_data.cpp
    tests/test_binary_serialization.cpp
    tests/test_telemetry.cpp
    tests/test_event_log.cpp
    tests/test_reference_matcher.cpp
    tests/test_engine_comparison.cpp
    tests/test_property_based.cpp
    tests/test_replay.cpp
)

target_link_libraries(run_tests PRIVATE
    lob_engine
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(run_tests)

# Benchmarks
add_executable(bench_throughput benchmarks/bench_throughput.cpp)
target_link_libraries(bench_throughput PRIVATE lob_engine)

add_executable(bench_market_sim benchmarks/bench_market_sim.cpp)
target_link_libraries(bench_market_sim PRIVATE lob_engine)

# Installation (optional)
install(TARGETS lob_engine
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/lob
    DESTINATION include
)

