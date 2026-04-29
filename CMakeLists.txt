cmake_minimum_required(VERSION 3.14)
project(tsdb VERSION 1.0 LANGUAGES CXX)

# Require C++17 for std::optional and structured bindings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings — treat code quality seriously
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Shared library of engine + utils ─────────────────────────────────────────
add_library(tsdb_core
    src/TimeSeries.cpp
    src/Parser.cpp
    src/Exporter.cpp
)
target_include_directories(tsdb_core PUBLIC include)

# ── Main REPL executable ──────────────────────────────────────────────────────
add_executable(tsdb src/main.cpp)
target_link_libraries(tsdb PRIVATE tsdb_core)

# ── Test executable ───────────────────────────────────────────────────────────
add_executable(tsdb_tests tests/tests.cpp)
target_link_libraries(tsdb_tests PRIVATE tsdb_core)

# Enable CTest support
enable_testing()
add_test(NAME unit_tests COMMAND tsdb_tests)
