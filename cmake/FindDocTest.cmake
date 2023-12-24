#********** Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake FindModule script for the 'DocTest' external project.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/module/FetchContent.html#fetchcontent
#**            for more on the 'FetchContent' command.
#**
#**            https://github.com/doctest/doctest
#**            for more on the 'DocTest' project.
#**
#**
#*****************************************************************************

include(FetchContent)

FetchContent_Declare(DocTest
  GIT_REPOSITORY  git@github.com:doctest/doctest.git
  GIT_TAG         v2.4.11
)

FetchContent_MakeAvailable(DocTest)

#*****************************************************************************
