//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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

  // TODO: Perhaps expand this?

  TEST_CASE("sc::token_equality") {
    rc::check("Given same args, Tokens should be identical",
              [](const SourceLocation &loc, const std::string &lexeme,
                 TokenType type) {
                auto token_a = Token(loc, type, lexeme);
                auto token_b = Token(loc, type, lexeme);
                RC_ASSERT(token_a.is_same_type_as(token_b));
                RC_ASSERT(token_a.is_same_lexeme_as(token_b));
                RC_ASSERT(token_a.is_same_type_lexeme_as(token_b));
                RC_ASSERT(token_a == token_b);
              });
  }

  TEST_CASE("sc::token_type_equality") {
    rc::check("type equality doesn't care about location or lexeme",
              [](const SourceLocation &loc_a, const SourceLocation &loc_b,
                 const std::string &lexeme_a, const std::string &lexeme_b,
                 TokenType type) {
                auto token_a = Token(loc_a, type, lexeme_a);
                auto token_b = Token(loc_b, type, lexeme_b);
                RC_ASSERT(token_a.is_same_type_as(token_b));
              });
  }

  TEST_CASE("sc::token_type_lexeme_equality") {
    rc::check("type_lexeme equality doesn't care about location",
              [](const SourceLocation &loc_a, const SourceLocation &loc_b,
                 const std::string &lexeme, TokenType type) {
                auto token_a = Token(loc_a, type, lexeme);
                auto token_b = Token(loc_b, type, lexeme);
                RC_ASSERT(token_a.is_same_type_as(token_b));
                RC_ASSERT(token_a.is_same_lexeme_as(token_b));
                RC_ASSERT(token_a.is_same_type_lexeme_as(token_b));
              });
  }
}
//****************************************************************************
} // namespace sc
//****************************************************************************
