//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for files 'source_location.hpp' and 'token.hpp'.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*            https://github.com/emil-e/rapidcheck
//*            for more on the 'RapidCheck' project.
//*
//****************************************************************************

#include "core_test_lib.hpp"      // For arbitraries
#include "sc/source_location.hpp" // For SourceLocation class
#include "sc/token.hpp"           // For Token class
#include <doctest/doctest.h>      // For doctest
#include <rapidcheck.h>           // For rapidcheck

//****************************************************************************
namespace sc {
//****************************************************************************
TEST_SUITE("core.token") {
  TEST_CASE("sc::token_equality") {
    rc::check("Given same args, Tokens should be identical",
              [](const SourceLocation &loc, TokenType type) {
                auto token_a = Token(loc, type);
                auto token_b = Token(loc, type);
                RC_ASSERT(token_a.get_token_type() == token_b.get_token_type());
                RC_ASSERT(token_a.get_location() == token_b.get_location());
              });
  }

  TEST_CASE("sc::token_type_equality") {
    rc::check("type equality doesn't care about location",
              [](const SourceLocation &loc_a, const SourceLocation &loc_b,
                 TokenType type) {
                auto token_a = Token(loc_a, type);
                auto token_b = Token(loc_b, type);
                RC_ASSERT(token_a.get_token_type() == token_b.get_token_type());
              });
  }
}
//****************************************************************************
} // namespace sc
//****************************************************************************
