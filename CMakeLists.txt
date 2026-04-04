#********** Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake build script for the 'interpreters' project.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/
#**            for more on the CMake language.
#**
#**            https://crascit.com/2015/03/28/enabling-cxx11-in-cmake/
#**            for more on setting the C++ standard.
#**
#**            https://stackoverflow.com/a/59108287
#**            for more on setting compiler options.
#**
#**
#*****************************************************************************

cmake_minimum_required(VERSION 3.25)

project(interpreters
  VERSION       0.1.0.0
  HOMEPAGE_URL  "https://github.com/seanwilliamcarroll/interpreters")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CTest)

find_package(DocTest       2.4.11 REQUIRED)
find_package(RapidCheck           REQUIRED)

set(CMAKE_CXX_STANDARD            23 )
set(CMAKE_CXX_STANDARD_REQUIRED   ON )
set(CMAKE_CXX_EXTENSIONS          OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )

set(PROJECT_WARNINGS
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wshadow
  -Wconversion
  -Wold-style-cast
  -Wnon-virtual-dtor
  -Woverloaded-virtual
)

option(ENABLE_ASAN "Enable AddressSanitizer for debug builds" OFF)

add_subdirectory(core)
add_subdirectory(blip)
add_subdirectory(bust)

#*****************************************************************************
