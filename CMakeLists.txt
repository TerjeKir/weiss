cmake_minimum_required(VERSION 3.10)

# Project
project(Weiss VERSION 1.0 LANGUAGES CXX)

# Flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow")
set(CMAKE_CXX_FLAGS "-pthread")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add the executable
add_executable(
    weiss

    src/bitboard.cpp
    src/board.cpp
    src/evaluate.cpp
    src/makemove.cpp
    src/move.cpp
    src/movegen.cpp
    src/movepicker.cpp
    src/psqt.cpp
    src/search.cpp
    src/tests.cpp
    src/threads.cpp
    src/time.cpp
    src/transposition.cpp
    src/uci.cpp

    src/pyrrhic/tbprobe.cpp
    src/noobprobe/noobprobe.cpp
    src/tuner/tuner.cpp
)
