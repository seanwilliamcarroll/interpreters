//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for the bust lexer.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*            https://github.com/emil-e/rapidcheck
//*            for more on the 'RapidCheck' project.
//*
//****************************************************************************

#include <exceptions.hpp>
#include <lexer.hpp>
#include <tokens.hpp>

#include <doctest/doctest.h>
#include <rapidcheck.h>
#include <sstream>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.lexer") {

  // --- Helpers -------------------------------------------------------------

  auto lex(const std::string &input) {
    std::istringstream stream(input);
    return make_lexer(stream);
  }

  void check_token_types(const std::string &input,
                         const std::vector<TokenType> &expected) {
    std::istringstream stream(input);
    auto lexer = make_lexer(stream);
    for (size_t i = 0; i < expected.size(); ++i) {
      auto token = lexer->get_next_token();
      CAPTURE(input);
      CAPTURE(i);
      CHECK(token != nullptr);
      INFO("expected: " << token_type_to_string(expected[i]) << " got: "
                        << token_type_to_string(token->get_token_type()));
      CHECK(token->get_token_type() == expected[i]);
    }
  }

  // --- Generators ----------------------------------------------------------

  auto whitespace_generator() {
    return rc::gen::apply(
        [](const std::vector<char> &chars) {
          return std::string(chars.begin(), chars.end());
        },
        rc::gen::suchThat(
            rc::gen::container<std::vector<char>>(
                rc::gen::element<char>(' ', '\t', '\n', '\r')),
            [](const std::vector<char> &v) { return v.size() < 10000; }));
  }

  auto nonzero_digit_generator() { return rc::gen::inRange('1', '9'); }

  auto digit_generator() { return rc::gen::inRange('0', '9'); }

  auto digits_generator() {
    return rc::gen::apply(
        [](const std::vector<char> &chars) {
          return std::string(chars.begin(), chars.end());
        },
        rc::gen::suchThat(
            rc::gen::container<std::vector<char>>(digit_generator()),
            [](const std::vector<char> &v) {
              return !v.empty() && v.size() < 10;
            }));
  }

  auto int_literal_generator() {
    return rc::gen::join(rc::gen::element(
        rc::gen::just<std::string>("0"),
        rc::gen::apply(
            [](char first, const std::string &rest) {
              return std::string(1, first) + rest;
            },
            nonzero_digit_generator(),
            rc::gen::apply(
                [](const std::vector<char> &chars) {
                  return std::string(chars.begin(), chars.end());
                },
                rc::gen::container<std::vector<char>>(digit_generator())))));
  }

  auto identifier_start_generator() {
    return rc::gen::element(rc::gen::inRange('a', 'z'), rc::gen::just('_'));
  }

  auto identifier_continue_generator() {
    return rc::gen::element(rc::gen::inRange('a', 'z'),
                            rc::gen::inRange('A', 'Z'),
                            rc::gen::inRange('0', '9'), rc::gen::just('_'));
  }

  // --- Empty / whitespace --------------------------------------------------

  TEST_CASE("bust::lexer_empty") {
    std::istringstream stream;
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token != nullptr);
    CHECK(token->get_token_type() == TokenType::EOF_TOKEN);
  }

  TEST_CASE("bust::lexer_whitespace") {
    rc::check("Whitespace-only input produces EOF", [] {
      std::istringstream stream(*whitespace_generator());
      auto lexer = make_lexer(stream);
      auto token = lexer->get_next_token();
      CHECK(token != nullptr);
      CHECK(token->get_token_type() == TokenType::EOF_TOKEN);
    });
  }

  // --- Single character tokens ---------------------------------------------

  TEST_CASE("bust::lexer_single_char_tokens") {
    struct Case {
      const char *input;
      TokenType type;
    };

    std::vector<Case> cases = {
        {")", TokenType::RPAREN},    {"{", TokenType::LBRACE},
        {"}", TokenType::RBRACE},    {":", TokenType::COLON},
        {";", TokenType::SEMICOLON}, {",", TokenType::COMMA},
        {"+", TokenType::PLUS},      {"*", TokenType::STAR},
        {"%", TokenType::PERCENT},
    };

    for (const auto &c : cases) {
      CAPTURE(c.input);
      check_token_types(c.input, {c.type, TokenType::EOF_TOKEN});
    }
  }

  // --- Two character tokens ------------------------------------------------

  TEST_CASE("bust::lexer_two_char_tokens") {
    struct Case {
      const char *input;
      TokenType type;
    };

    std::vector<Case> cases = {
        {"->", TokenType::ARROW},      {"==", TokenType::EQ_EQ},
        {"!=", TokenType::BANG_EQ},    {"<=", TokenType::LESS_EQ},
        {">=", TokenType::GREATER_EQ}, {"||", TokenType::OR_OR},
        {"&&", TokenType::AND_AND},    {"()", TokenType::UNIT},
    };

    for (const auto &c : cases) {
      CAPTURE(c.input);
      check_token_types(c.input, {c.type, TokenType::EOF_TOKEN});
    }
  }

  // --- Single-or-double disambiguation -------------------------------------

  TEST_CASE("bust::lexer_single_when_not_double") {
    struct Case {
      const char *input;
      TokenType first_type;
      TokenType second_type;
    };

    std::vector<Case> cases = {
        {"= ", TokenType::EQUALS, TokenType::EOF_TOKEN},
        {"! ", TokenType::BANG, TokenType::EOF_TOKEN},
        {"| ", TokenType::PIPE, TokenType::EOF_TOKEN},
        {"< ", TokenType::LESS, TokenType::EOF_TOKEN},
        {"> ", TokenType::GREATER, TokenType::EOF_TOKEN},
        {"& ", TokenType::AND, TokenType::EOF_TOKEN},
        {"- ", TokenType::MINUS, TokenType::EOF_TOKEN},
        {"( ", TokenType::LPAREN, TokenType::EOF_TOKEN},
    };

    for (const auto &c : cases) {
      CAPTURE(c.input);
      check_token_types(c.input, {c.first_type, c.second_type});
    }
  }

  // --- Integer literals ----------------------------------------------------

  TEST_CASE("bust::lexer_zero") {
    check_token_types("0", {TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_integers") {
    rc::check("Valid integer literals lex to INT_LITERAL", [] {
      std::istringstream stream(*int_literal_generator());
      auto lexer = make_lexer(stream);

      auto token = lexer->get_next_token();
      CHECK(token != nullptr);
      CHECK(token->get_token_type() == TokenType::INT_LITERAL);

      auto eof = lexer->get_next_token();
      CHECK(eof != nullptr);
      CHECK(eof->get_token_type() == TokenType::EOF_TOKEN);
    });
  }

  TEST_CASE("bust::lexer_integer_value") {
    std::istringstream stream("42");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token != nullptr);
    CHECK(token->get_token_type() == TokenType::INT_LITERAL);
    auto *num_token = dynamic_cast<const TokenNumber *>(token.get());
    CHECK(num_token != nullptr);
    CHECK(num_token->get_value() == "42");
  }

  TEST_CASE("bust::lexer_integer_preserves_large_numbers") {
    // Larger than INT64_MAX — lexer stores as string, not parsed yet
    std::istringstream stream("10000000000000000000");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token != nullptr);
    CHECK(token->get_token_type() == TokenType::INT_LITERAL);
    auto *num_token = dynamic_cast<const TokenNumber *>(token.get());
    CHECK(num_token != nullptr);
    CHECK(num_token->get_value() == "10000000000000000000");
  }

  TEST_CASE("bust::lexer_integer_value_zero") {
    std::istringstream stream("0");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token != nullptr);
    auto *num_token = dynamic_cast<const TokenNumber *>(token.get());
    CHECK(num_token != nullptr);
    CHECK(num_token->get_value() == "0");
  }

  // --- Keywords ------------------------------------------------------------

  TEST_CASE("bust::lexer_keywords") {
    for (const auto &[input, type] : keywords) {
      CAPTURE(input);
      check_token_types(std::string(input), {type, TokenType::EOF_TOKEN});
    }
  }

  TEST_CASE("bust::lexer_keyword_prefix_is_identifier") {
    // "iffy" should be IDENTIFIER, not IF
    check_token_types("iffy", {TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  // --- Identifiers ---------------------------------------------------------

  TEST_CASE("bust::lexer_simple_identifiers") {
    std::vector<const char *> cases = {"x", "foo", "bar_baz", "_hidden",
                                       "my_var2"};

    for (const auto &input : cases) {
      CAPTURE(input);
      check_token_types(input, {TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
    }
  }

  // --- Comments ------------------------------------------------------------

  TEST_CASE("bust::lexer_line_comment") {
    check_token_types("// this is a comment\n42",
                      {TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_line_comment_eof") {
    check_token_types("// comment at end of file", {TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_block_comment") {
    check_token_types("/* block */ 42",
                      {TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_block_comment_multiline") {
    check_token_types("/* block\n   comment */ 42",
                      {TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_slash_not_comment") {
    check_token_types("/ 42", {TokenType::SLASH, TokenType::INT_LITERAL,
                               TokenType::EOF_TOKEN});
  }

  // --- Compound sequences --------------------------------------------------

  TEST_CASE("bust::lexer_let_binding") {
    check_token_types("let x: i64 = 42;",
                      {TokenType::LET, TokenType::IDENTIFIER, TokenType::COLON,
                       TokenType::I64, TokenType::EQUALS,
                       TokenType::INT_LITERAL, TokenType::SEMICOLON,
                       TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_function_def") {
    check_token_types("fn main() -> i64 { 0 }",
                      {TokenType::FN, TokenType::IDENTIFIER, TokenType::UNIT,
                       TokenType::ARROW, TokenType::I64, TokenType::LBRACE,
                       TokenType::INT_LITERAL, TokenType::RBRACE,
                       TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_if_expression") {
    check_token_types(
        "if x == 0 { 1 } else { 2 }",
        {TokenType::IF, TokenType::IDENTIFIER, TokenType::EQ_EQ,
         TokenType::INT_LITERAL, TokenType::LBRACE, TokenType::INT_LITERAL,
         TokenType::RBRACE, TokenType::ELSE, TokenType::LBRACE,
         TokenType::INT_LITERAL, TokenType::RBRACE, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_arithmetic") {
    check_token_types(
        "a + b * c - d / e % f",
        {TokenType::IDENTIFIER, TokenType::PLUS, TokenType::IDENTIFIER,
         TokenType::STAR, TokenType::IDENTIFIER, TokenType::MINUS,
         TokenType::IDENTIFIER, TokenType::SLASH, TokenType::IDENTIFIER,
         TokenType::PERCENT, TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_logical") {
    check_token_types("a && b || !c",
                      {TokenType::IDENTIFIER, TokenType::AND_AND,
                       TokenType::IDENTIFIER, TokenType::OR_OR, TokenType::BANG,
                       TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_lambda") {
    check_token_types("|x, y| -> i64 { x + y }",
                      {TokenType::PIPE, TokenType::IDENTIFIER, TokenType::COMMA,
                       TokenType::IDENTIFIER, TokenType::PIPE, TokenType::ARROW,
                       TokenType::I64, TokenType::LBRACE, TokenType::IDENTIFIER,
                       TokenType::PLUS, TokenType::IDENTIFIER,
                       TokenType::RBRACE, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_comparison_chain") {
    check_token_types("a < b >= c != d",
                      {TokenType::IDENTIFIER, TokenType::LESS,
                       TokenType::IDENTIFIER, TokenType::GREATER_EQ,
                       TokenType::IDENTIFIER, TokenType::BANG_EQ,
                       TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  // --- Error cases -----------------------------------------------------------

  TEST_CASE("bust::lexer_invalid_character") {
    std::istringstream stream("@");
    auto lexer = make_lexer(stream);
    CHECK_THROWS_AS(lexer->get_next_token(), core::CompilerException);
  }

  TEST_CASE("bust::lexer_invalid_character_hash") {
    std::istringstream stream("#");
    auto lexer = make_lexer(stream);
    CHECK_THROWS_AS(lexer->get_next_token(), core::CompilerException);
  }

  TEST_CASE("bust::lexer_unterminated_block_comment") {
    std::istringstream stream("/* unterminated");
    auto lexer = make_lexer(stream);
    CHECK_THROWS_AS(lexer->get_next_token(), core::CompilerException);
  }

  // --- Source location tracking ----------------------------------------------

  TEST_CASE("bust::lexer_source_location_first_token") {
    std::istringstream stream("42");
    auto lexer = make_lexer(stream, "test.bu");
    auto token = lexer->get_next_token();
    CHECK(token->get_location().line == 1);
    // Column is 0-based: first char hasn't been advanced yet
    CHECK(token->get_location().column == 0);
  }

  TEST_CASE("bust::lexer_source_location_after_whitespace") {
    std::istringstream stream("   x");
    auto lexer = make_lexer(stream, "test.bu");
    auto token = lexer->get_next_token();
    CHECK(token->get_location().line == 1);
    // 3 spaces consumed, column is 3
    CHECK(token->get_location().column == 3);
  }

  TEST_CASE("bust::lexer_source_location_second_line") {
    std::istringstream stream("x\ny");
    auto lexer = make_lexer(stream, "test.bu");
    lexer->get_next_token(); // x
    auto token = lexer->get_next_token();
    CHECK(token->get_location().line == 2);
    CHECK(token->get_location().column == 0);
  }

  // --- Tokens without whitespace ---------------------------------------------

  TEST_CASE("bust::lexer_adjacent_tokens_no_whitespace") {
    check_token_types("1+2", {TokenType::INT_LITERAL, TokenType::PLUS,
                              TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_parens_around_identifier") {
    check_token_types("(foo)", {TokenType::LPAREN, TokenType::IDENTIFIER,
                                TokenType::RPAREN, TokenType::EOF_TOKEN});
  }

  // --- Leading zeros ---------------------------------------------------------

  TEST_CASE("bust::lexer_leading_zero_is_single_token") {
    // 007 should lex as "0" then "07" is NOT valid — it should be 0, 0, 7
    // Actually the lexer treats 0 as a complete int literal (no multi-digit
    // starting with 0), so "007" → INT(0), INT(0), INT(7)
    check_token_types("007", {TokenType::INT_LITERAL, TokenType::INT_LITERAL,
                              TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  // --- Identifier with uppercase letters -------------------------------------

  TEST_CASE("bust::lexer_uppercase_identifier") {
    check_token_types("MyVar", {TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_mixed_case_identifier") {
    check_token_types("camelCase",
                      {TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  // --- Multiple comments -----------------------------------------------------

  TEST_CASE("bust::lexer_consecutive_comments") {
    check_token_types("// first\n// second\n42",
                      {TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_block_comment_with_stars") {
    // Block comment with extra * inside should still work
    check_token_types("/*** comment ***/42",
                      {TokenType::INT_LITERAL, TokenType::EOF_TOKEN});
  }

  // --- Keyword prefix disambiguation
  // ------------------------------------------

  TEST_CASE("bust::lexer_as_keyword_not_identifier_prefix") {
    check_token_types("asdf", {TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_i8_not_identifier_prefix") {
    check_token_types("i8x", {TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_char_not_identifier_prefix") {
    check_token_types("charcoal",
                      {TokenType::IDENTIFIER, TokenType::EOF_TOKEN});
  }

  // --- Char literals ---------------------------------------------------------

  TEST_CASE("bust::lexer_char_literal_simple") {
    check_token_types("'A'", {TokenType::CHAR_LITERAL, TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_char_literal_value") {
    std::istringstream stream("'A'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token != nullptr);
    CHECK(char_token->get_value() == "'A'");
  }

  TEST_CASE("bust::lexer_char_literal_digit") {
    std::istringstream stream("'9'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token != nullptr);
    CHECK(char_token->get_value() == "'9'");
  }

  TEST_CASE("bust::lexer_char_literal_space") {
    std::istringstream stream("' '");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token != nullptr);
    CHECK(char_token->get_value() == "' '");
  }

  TEST_CASE("bust::lexer_char_literal_tilde") {
    std::istringstream stream("'~'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
  }

  // --- Char literal escape sequences -----------------------------------------

  TEST_CASE("bust::lexer_char_literal_escape_newline") {
    std::istringstream stream("'\\n'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\n'");
  }

  TEST_CASE("bust::lexer_char_literal_escape_tab") {
    std::istringstream stream("'\\t'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\t'");
  }

  TEST_CASE("bust::lexer_char_literal_escape_carriage_return") {
    std::istringstream stream("'\\r'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\r'");
  }

  TEST_CASE("bust::lexer_char_literal_escape_backslash") {
    std::istringstream stream("'\\\\'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\\\'");
  }

  TEST_CASE("bust::lexer_char_literal_escape_single_quote") {
    std::istringstream stream("'\\''");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\''");
  }

  TEST_CASE("bust::lexer_char_literal_escape_null") {
    std::istringstream stream("'\\0'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\0'");
  }

  // --- Char literal hex escapes ----------------------------------------------

  TEST_CASE("bust::lexer_char_literal_hex_escape") {
    std::istringstream stream("'\\x41'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\x41'");
  }

  TEST_CASE("bust::lexer_char_literal_hex_escape_lowercase") {
    std::istringstream stream("'\\xff'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\xff'");
  }

  TEST_CASE("bust::lexer_char_literal_hex_escape_zero") {
    std::istringstream stream("'\\x00'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\x00'");
  }

  TEST_CASE("bust::lexer_char_literal_hex_escape_mixed_case") {
    std::istringstream stream("'\\xAf'");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token->get_token_type() == TokenType::CHAR_LITERAL);
    auto *char_token = dynamic_cast<const TokenChar *>(token.get());
    CHECK(char_token->get_value() == "'\\xAf'");
  }

  // --- Char literal errors ---------------------------------------------------

  TEST_CASE("bust::lexer_char_literal_empty") {
    std::istringstream stream("''");
    auto lexer = make_lexer(stream);
    CHECK_THROWS_AS(lexer->get_next_token(), core::CompilerException);
  }

  TEST_CASE("bust::lexer_char_literal_unterminated") {
    std::istringstream stream("'A");
    auto lexer = make_lexer(stream);
    CHECK_THROWS_AS(lexer->get_next_token(), core::CompilerException);
  }

  TEST_CASE("bust::lexer_char_literal_invalid_escape") {
    std::istringstream stream("'\\z'");
    auto lexer = make_lexer(stream);
    CHECK_THROWS_AS(lexer->get_next_token(), core::CompilerException);
  }

  TEST_CASE("bust::lexer_char_literal_hex_escape_incomplete") {
    std::istringstream stream("'\\xA'");
    auto lexer = make_lexer(stream);
    CHECK_THROWS_AS(lexer->get_next_token(), core::CompilerException);
  }

  // --- Char literal in context -----------------------------------------------

  TEST_CASE("bust::lexer_char_literal_in_let_binding") {
    check_token_types("let c: char = 'A';",
                      {TokenType::LET, TokenType::IDENTIFIER, TokenType::COLON,
                       TokenType::CHAR, TokenType::EQUALS,
                       TokenType::CHAR_LITERAL, TokenType::SEMICOLON,
                       TokenType::EOF_TOKEN});
  }

  TEST_CASE("bust::lexer_cast_expression") {
    check_token_types("'A' as i32", {TokenType::CHAR_LITERAL, TokenType::AS,
                                     TokenType::I32, TokenType::EOF_TOKEN});
  }

  // --- Identifier value preservation -----------------------------------------

  TEST_CASE("bust::lexer_identifier_value") {
    std::istringstream stream("my_var2");
    auto lexer = make_lexer(stream);
    auto token = lexer->get_next_token();
    CHECK(token != nullptr);
    CHECK(token->get_token_type() == TokenType::IDENTIFIER);
    auto *id_token = dynamic_cast<const TokenIdentifier *>(token.get());
    CHECK(id_token != nullptr);
    CHECK(id_token->get_value() == "my_var2");
  }
}
//****************************************************************************
} // namespace bust
//****************************************************************************
