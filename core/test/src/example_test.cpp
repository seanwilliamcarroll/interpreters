//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for file 'example.hpp'.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*            https://github.com/emil-e/rapidcheck
//*            for more on the 'RapidCheck' project.
//*
//****************************************************************************

#include "sc/example.hpp"    // For example
#include <doctest/doctest.h> // For doctest
#include <rapidcheck.h>      // For rapidcheck

//****************************************************************************
namespace sc {
//****************************************************************************
TEST_SUITE("core.example") {

  TEST_CASE("sc::example") {
    // A simple doctest assertion:

    CHECK(example(1) == 3);

    // A simple RapidCheck property check:

    rc::check("∀i ∈ ℤ: example(i) == i * 3",
              [](int i) { return example(i) == i * 3; });
  }
}
//****************************************************************************
} // namespace sc
//****************************************************************************
