//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for files 'core_lexer.hpp'.
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
#include "sc/lexer_interface.hpp" // For LexerInterface
#include "sc/sc.hpp"              // For make_lexer function
#include "sc/token.hpp"           // For Token class
#include <doctest/doctest.h>      // For doctest
#include <iostream>
#include <rapidcheck.h> // For rapidcheck
#include <sstream>      // For stringstream

//****************************************************************************
namespace sc {
//****************************************************************************
TEST_SUITE("core.lexer") {

  // FIXME: Add tests
  std::vector<std::unique_ptr<Token>> get_all_tokens(LexerInterface * lexer) {
    std::vector<std::unique_ptr<Token>> tokens;

    std::unique_ptr<Token> token = lexer->get_next_token();
    CHECK(token != nullptr);
    while (token->get_token_type() != Token::EOF_TOKENTYPE) {
      tokens.push_back(std::move(token));
      token = lexer->get_next_token();
      CHECK(token != nullptr);
    }
    // Last EOF token
    tokens.push_back(std::move(token));
    return tokens;
  }

  void add_whitespace_stream(std::stringstream & in_stream,
                             const unsigned int num_chars) {
    for (unsigned int char_index = 0; char_index < num_chars; ++char_index) {
      in_stream << *rc::gen::element(' ', '\t', '\n', '\r');
    }
  }

  TEST_CASE("sc::lexer_empty") {
    std::istringstream in_stream;
    auto lexer = make_lexer(in_stream, {});
    auto tokens = get_all_tokens(lexer.get());
    CHECK(tokens.size() == 1);
    CHECK(tokens.at(0)->get_token_type() == Token::EOF_TOKENTYPE);
  }

  TEST_CASE("sc::lexer_whitespace") {
    rc::check("Given stream of whitespace, return only 1 token ", [] {
      std::stringstream in_stream;
      // Generate random stream of whitespace characters
      const unsigned int num_chars = *rc::gen::inRange(0, 10000);
      add_whitespace_stream(in_stream, num_chars);
      auto lexer = make_lexer(in_stream, {});
      auto tokens = get_all_tokens(lexer.get());
      CHECK(tokens.size() == 1);
      CHECK(tokens.at(tokens.size() - 1)->get_token_type() ==
            Token::EOF_TOKENTYPE);
    });
  }
}
//****************************************************************************
} // namespace sc
//****************************************************************************
