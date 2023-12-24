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

cmake_minimum_required(VERSION 3.20.2)

project(interpreters
  VERSION       0.1.0.0
  HOMEPAGE_URL  "https://github.com/seanwilliamcarroll/interpreters")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CTest)

find_package(DocTest       2.4.11 REQUIRED)
find_package(RapidCheck           REQUIRED)

set(CMAKE_CXX_STANDARD            20 )
set(CMAKE_CXX_STANDARD_REQUIRED   ON )
set(CMAKE_CXX_EXTENSIONS          OFF)

add_compile_options(-Wall)

add_subdirectory(core)
add_subdirectory(blip)

#*****************************************************************************
