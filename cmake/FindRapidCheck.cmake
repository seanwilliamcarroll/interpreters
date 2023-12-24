#********** Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake FindModule script for the 'RapidCheck' external project.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/module/FetchContent.html#fetchcontent
#**            for more on the 'FetchContent' command.
#**
#**            https://github.com/emil-e/rapidcheck
#**            for more on the 'RapidCheck' project.
#**
#**
#*****************************************************************************

include(FetchContent)

FetchContent_Declare(RapidCheck
  GIT_REPOSITORY  git@github.com:emil-e/rapidcheck.git
  GIT_TAG         a5724ea5b0b00147109b0605c377f1e54c353ba2 # 04/16/2023
)

FetchContent_MakeAvailable(RapidCheck)

#*****************************************************************************
