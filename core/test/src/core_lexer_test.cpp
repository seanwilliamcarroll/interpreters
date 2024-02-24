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

  auto any_char_but_generator(auto generator, char char_to_skip) {
    return rc::gen::suchThat(generator, [char_to_skip](char input_char) {
      return input_char != char_to_skip;
    });
  }

  auto any_char_but_generator(auto generator,
                              const std::vector<char> &chars_to_skip) {
    return rc::gen::suchThat(generator, [&chars_to_skip](char input_char) {
      return std::none_of(chars_to_skip.begin(), chars_to_skip.end(),
                          [input_char](char inner_input_char) {
                            return input_char == inner_input_char;
                          });
    });
  }

  auto skip_range_generator(auto generator, char char_begin_inclusive,
                            char char_end_inclusive) {
    return rc::gen::suchThat(
        generator, [char_begin_inclusive, char_end_inclusive](char input_char) {
          return input_char >= char_begin_inclusive &&
                 input_char <= char_end_inclusive;
        });
  }

  // FIXME: Probably shouldn't need this
  auto ascii_characters_generator() {
    return skip_range_generator(rc::gen::character<char>(), 32, 127);
  }

  auto concat_char_generator(auto generator) {
    return rc::gen::apply(
        [](const std::vector<char> &char_vector) {
          return std::string(char_vector.begin(), char_vector.end());
        },
        generator);
  }

  auto whitespace_generator() {
    return concat_char_generator(
        rc::gen::suchThat(rc::gen::container<std::vector<char>>(
                              rc::gen::element<char>(' ', '\t', '\n', '\r')),
                          [](const std::vector<char> &char_vector) {
                            return char_vector.size() < 10000;
                          }));
  }

  auto random_digit_generator() { return rc::gen::inRange('0', '9'); }

  auto random_digits_generator() {
    // Want to make sure we at least get one digit from this generator, but not
    // larger than 9 digits
    return rc::gen::apply(
        [](const std::vector<char> &char_vector) {
          return std::string(char_vector.begin(), char_vector.end());
        },
        rc::gen::suchThat(
            rc::gen::container<std::vector<char>>(random_digit_generator()),
            [](const std::vector<char> &char_vector) {
              return char_vector.size() > 0 && char_vector.size() < 10;
            }));
  }

  auto digits_before_decimal_generator() {
    return rc::gen::join(rc::gen::element(
        rc::gen::just<std::string>("0"),
        rc::gen::suchThat(
            random_digits_generator(), [](const std::string &input_string) {
              return input_string.size() > 0 && input_string.at(0) != '0';
            })));
  }

  auto minus_generator() { return rc::gen::just<std::string>("-"); }

  auto optional_null_string(auto input_gen) {
    return rc::gen::join<std::string>(
        rc::gen::element(input_gen, rc::gen::just<std::string>("")));
  }

  auto optional_minus_generator() {
    return optional_null_string(minus_generator());
  }

  auto fractional_generator() {
    return rc::gen::apply(
        [](const std::string &input_string) {
          return std::string(".") + input_string;
        },
        random_digits_generator());
  }

  auto exponential_generator() {
    // Not perfect, hardcoded number to keep doubles in range
    return rc::gen::apply(
        [](const std::string &e_term, const std::string &unary_term,
           unsigned int input) {
          return e_term + unary_term + std::to_string(input);
        },
        rc::gen::element<std::string>("e", "E"),
        rc::gen::element<std::string>("", "+", "-"),
        rc::gen::inRange<unsigned int>(0, 300));
  }

  // Test Cases

  TEST_CASE("sc::lexer_empty") {
    std::istringstream in_stream;
    auto lexer = make_lexer(in_stream, {});
    auto token = lexer->get_next_token();
    CHECK(token != nullptr);
    CHECK(token->get_token_type() == Token::EOF_TOKENTYPE);
  }

  TEST_CASE("sc::lexer_whitespace") {
    rc::check("Given stream of whitespace, return only 1 token", [] {
      std::stringstream in_stream;
      in_stream << *whitespace_generator();

      auto lexer = make_lexer(in_stream, {});

      auto token = lexer->get_next_token();
      CHECK(token != nullptr);
      CHECK(token->get_token_type() == Token::EOF_TOKENTYPE);
    });
  }

  TEST_CASE("sc::lexer_integers") {
    rc::check("Test lexing of different integer numbers", [] {
      // Combinator to test different forms of numbers based on json parsing
      // spec

      // Build up each branching point as generators
      std::stringstream in_stream;
      in_stream << *optional_minus_generator()
                << *digits_before_decimal_generator();

      auto lexer = make_lexer(in_stream, {});

      auto first_token = lexer->get_next_token();
      CHECK(first_token != nullptr);
      CHECK(first_token->get_token_type() == Token::INT_LITERAL);

      auto second_token = lexer->get_next_token();
      CHECK(second_token != nullptr);
      CHECK(second_token->get_token_type() == Token::EOF_TOKENTYPE);
    });
  }

  TEST_CASE("sc::lexer_doubles_fractional") {
    rc::check("Test lexing of different double numbers: containing decimal",
              [] {
                // Combinator to test different forms of numbers based on json
                // parsing spec

                // Build up each branching point as generators

                std::stringstream in_stream;
                in_stream << *optional_minus_generator()
                          << *digits_before_decimal_generator()
                          << *fractional_generator()
                          << *optional_null_string(exponential_generator());

                auto lexer = make_lexer(in_stream, {});

                auto first_token = lexer->get_next_token();
                CHECK(first_token != nullptr);
                CHECK(first_token->get_token_type() == Token::DOUBLE_LITERAL);

                auto second_token = lexer->get_next_token();
                CHECK(second_token != nullptr);
                CHECK(second_token->get_token_type() == Token::EOF_TOKENTYPE);
              });
  }

  TEST_CASE("sc::lexer_doubles_exponential") {
    rc::check("Test lexing of different double numbers: containing exponential",
              [] {
                // Combinator to test different forms of numbers based on json
                // parsing spec

                // Build up each branching point as generators

                std::stringstream in_stream;
                in_stream << *optional_minus_generator()
                          << *digits_before_decimal_generator()
                          << *optional_null_string(fractional_generator())
                          << *exponential_generator();

                auto lexer = make_lexer(in_stream, {});

                auto first_token = lexer->get_next_token();
                CHECK(first_token != nullptr);
                CHECK(first_token->get_token_type() == Token::DOUBLE_LITERAL);

                auto second_token = lexer->get_next_token();
                CHECK(second_token != nullptr);
                CHECK(second_token->get_token_type() == Token::EOF_TOKENTYPE);
              });
  }

  TEST_CASE("sc::lexer_parends") {
    std::stringstream in_stream;
    in_stream << "()";
    auto lexer = make_lexer(in_stream, {});
    for (const auto &token_type :
         {Token::LEFT_PAREND, Token::RIGHT_PAREND, Token::EOF_TOKENTYPE}) {
      auto token = lexer->get_next_token();
      CHECK(token != nullptr);
      CHECK(token->get_token_type() == token_type);
    }
  }

  TEST_CASE("sc::block_commnet") {
    rc::check("Given stream of valid block_commnet, return only 1 token", [] {
      std::stringstream in_stream;
      in_stream << ";-";
      // FIXME: Not handling unicode correctly?
      in_stream << *concat_char_generator(rc::gen::container<std::vector<char>>(
          any_char_but_generator(ascii_characters_generator(), ';')));
      in_stream << "-;";

      auto lexer = make_lexer(in_stream, {});

      auto token = lexer->get_next_token();
      CHECK(token != nullptr);
      CHECK(token->get_token_type() == Token::EOF_TOKENTYPE);
    });
  }

  TEST_CASE("sc::string") {
    rc::check(
        "Given stream of valid string, return only 2 tokens, STRING_LITERAL "
        "and EOF_TOKENTYPE, without escape characters",
        [] {
          // FIXME: Not handling unicode correctly?
          std::stringstream in_stream;
          in_stream << "\"";
          in_stream << *concat_char_generator(
              rc::gen::container<std::vector<char>>(any_char_but_generator(
                  ascii_characters_generator(), {'"', '\\'})));
          in_stream << "\"";

          auto lexer = make_lexer(in_stream, {});

          auto first_token = lexer->get_next_token();
          CHECK(first_token != nullptr);
          CHECK(first_token->get_token_type() == Token::STRING_LITERAL);

          auto second_token = lexer->get_next_token();
          CHECK(second_token != nullptr);
          CHECK(second_token->get_token_type() == Token::EOF_TOKENTYPE);
        });
  }
}
//****************************************************************************
} // namespace sc
//****************************************************************************
