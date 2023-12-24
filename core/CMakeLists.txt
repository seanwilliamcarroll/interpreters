#********** Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake build script for the 'core' library.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/
#**            for more on the CMake language.
#**
#**
#*****************************************************************************

project(core VERSION 0.1.0)

#*****************************************************************************

add_library(core STATIC
  src/example.cpp src/token.cpp)

target_include_directories(core PUBLIC inc)

#*****************************************************************************

add_executable(core-test
  test/main.cpp
  test/example_test.cpp)

target_link_libraries(core-test
  core
  rapidcheck
  doctest::doctest)

#*****************************************************************************

add_test(NAME core.example COMMAND core-test -ts=core.example)

#*****************************************************************************
