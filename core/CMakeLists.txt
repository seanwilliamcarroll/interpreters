#********** Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake build script for the 'core' header-only library.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/
#**            for more on the CMake language.
#**
#**
#*****************************************************************************

project(core VERSION 0.1.0)

#*****************************************************************************

add_library(core INTERFACE)

target_include_directories(core INTERFACE inc)

target_compile_options(core INTERFACE ${PROJECT_WARNINGS})

#*****************************************************************************
