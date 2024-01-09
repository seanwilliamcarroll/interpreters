#***** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : Platform specific tweaks and fixes.
#**
#**
#*****************************************************************************

# The default CMake kit for the MacPorts Clang compiler seems to be unaware of
# the location in which its standard C++ library is found:

if (APPLE AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")

 if    (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "16" AND
        ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS          "17"    )

   message(STATUS "Using the MacPorts clang-16 standard c++ library")
   set(CMAKE_CXX_STANDARD_LIBRARIES  "/opt/local/libexec/llvm-16/lib/libc++.dylib"
   CACHE STRING                      "The MacPorts clang-16 standard c++ li brary")

 elseif(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "17" AND
        ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS          "18"    )

   message(STATUS "Using the MacPorts clang-17 standard c++ library")
   set(CMAKE_CXX_STANDARD_LIBRARIES  "/opt/local/libexec/llvm-17/lib/libc++.dylib"
   CACHE STRING                      "The MacPorts clang-17 standard c++ library")

 endif()

endif()

#*****************************************************************************
