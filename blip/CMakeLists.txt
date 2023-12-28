#********** Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake build script for the 'blip' executable.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/
#**            for more on the CMake language.
#**
#**
#*****************************************************************************

project(blip VERSION 0.1.0)

add_executable(blip src/main.cpp)

add_library(bliplib STATIC
  src/blip.cpp src/lexer.cpp)

target_include_directories(bliplib PUBLIC inc)

target_link_libraries(bliplib core)

target_link_libraries(blip bliplib)

#****************************************************************************
