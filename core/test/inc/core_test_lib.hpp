//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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
    return gen::build<sc::SourceLocation>(
        gen::set(&sc::SourceLocation::m_line),
        gen::set(&sc::SourceLocation::m_column),
        gen::set(&sc::SourceLocation::m_file_name),
        gen::set(&sc::SourceLocation::m_function_name));
  }
};

template <> struct Arbitrary<sc::Token> {
  static Gen<sc::Token> arbitrary() {
    return gen::construct<sc::Token>(gen::arbitrary<sc::SourceLocation>(),
                                     gen::arbitrary<sc::TokenType>(),
                                     gen::arbitrary<std::string>());
  }
};

//****************************************************************************
} // namespace rc
//****************************************************************************
