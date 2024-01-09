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

add_executable(blip-exe src/main.cpp)

set_target_properties(blip-exe PROPERTIES OUTPUT_NAME blip)

add_library(blip-lib STATIC
  src/blip.cpp)

set_target_properties(blip-lib PROPERTIES OUTPUT_NAME blip)

target_include_directories(blip-lib PUBLIC inc)

target_link_libraries(blip-lib core)

target_link_libraries(blip-exe blip-lib)

if (APPLE)
  set_target_properties(blip-exe PROPERTIES
    BUILD_RPATH "/opt/local/libexec/llvm-17/lib"
  )
endif(APPLE)

#****************************************************************************
