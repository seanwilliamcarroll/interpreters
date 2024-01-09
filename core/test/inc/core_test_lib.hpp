//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Test collateral for core tests
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*            https://github.com/emil-e/rapidcheck
//*            for more on the 'RapidCheck' project.
//*
//****************************************************************************

#include "sc/source_location.hpp" // For SourceLocation class
#include "sc/token.hpp"           // For Token class
#include <doctest/doctest.h>      // For doctest
#include <rapidcheck.h>           // For rapidcheck

//****************************************************************************
namespace rc {
//****************************************************************************

template <> struct Arbitrary<sc::SourceLocation> {
  static Gen<sc::SourceLocation> arbitrary() {
    return gen::construct<sc::SourceLocation>(
        // This is hacky, but not sure of a better way
        gen::map(gen::arbitrary<std::string>(),
                 [](const std::string &in_str) { return in_str.c_str(); }),
        gen::arbitrary<unsigned int>(), gen::arbitrary<unsigned int>());
  }
};

template <> struct Arbitrary<sc::Token> {
  static Gen<sc::Token> arbitrary() {
    return gen::construct<sc::Token>(gen::arbitrary<sc::SourceLocation>(),
                                     gen::arbitrary<sc::TokenType>());
  }
};

//****************************************************************************
} // namespace rc
//****************************************************************************
