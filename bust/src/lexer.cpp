//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Lexer implementation for bust.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <lexer.hpp>
#include <source_location.hpp>
#include <token.hpp>
#include <tokens.hpp>

#include <array>
#include <cctype>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

//****************************************************************************
namespace bust {
namespace {
//****************************************************************************

bool is_digit(char character) { return std::isdigit(character) != 0; }

bool is_spacing_char(char character) {
  return character == ' ' || character == '\t';
}

bool is_line_terminator_char(char character) {
  return character == '\n' || character == '\r';
}

bool is_lower(char character) { return character >= 'a' && character <= 'z'; }

bool is_upper(char character) { return character >= 'A' && character <= 'Z'; }

bool is_single_quote(char character) { return character == '\''; }

bool is_slash(char character) { return character == '/'; }

bool is_printable_char(char character) {
  // skip single quote and backslash so they are parsed correctly
  return character >= ' ' && character <= '~' && character != '\'' &&
         character != '\\';
}

bool is_escape_sequence_terminator(char character) {
  switch (character) {
  case 'n':
  case 't':
  case 'r':
  case '\\':
  case '\'':
  case '0':
    return true;
  default:
    return false;
  }
}

bool is_hex_character(char character) {
  return (character >= '0' && character <= '9') ||
         (character >= 'a' && character <= 'f') ||
         (character >= 'A' && character <= 'F');
}

struct Lexer : LexerInterface {

  Lexer(std::istream &in_stream, const char *hint)
      : m_in_stream(in_stream), m_keywords(keywords.begin(), keywords.end()),
        m_hint(hint) {}

  [[nodiscard]] auto make_token(TokenType t) const {
    return std::make_unique<Token>(get_current_loc(), t);
  }

  [[nodiscard]] static auto make_token(const core::SourceLocation &l,
                                       TokenType t) {
    return std::make_unique<Token>(l, t);
  }

  template <class V>
  [[nodiscard]] auto make_token(const core::SourceLocation &l, TokenType t,
                                const V &v) const {
    return std::make_unique<core::TokenOf<TokenType, V>>(l, t, v);
  }

  template <class... args> [[noreturn]] void on_error(args &&...a) const {

    std::ostringstream o; // Local stringstream

    (o << ... << a); // Fold operator <<

    throw core::CompilerException("LexerException", o.str(), get_current_loc());
  }

  std::unique_ptr<Token> try_single_char_token(char character) {
    const static std::unordered_map<char, TokenType>
        exclusively_single_char_mapping = {
            {')', TokenType::RPAREN},    {'{', TokenType::LBRACE},
            {'}', TokenType::RBRACE},    {':', TokenType::COLON},
            {';', TokenType::SEMICOLON}, {',', TokenType::COMMA},
            {'+', TokenType::PLUS},      {'*', TokenType::STAR},
            {'%', TokenType::PERCENT},   {'.', TokenType::DOT},
        };

    auto iter = exclusively_single_char_mapping.find(character);

    if (iter == exclusively_single_char_mapping.end()) {
      return {};
    }
    return single_char_token(iter->second);
  }

  std::unique_ptr<Token> try_single_or_double_char_token(char character) {
    struct SingleDoubleChoice {
      std::string m_two_character_token;
      TokenType m_single_token;
      TokenType m_double_token;
    };
    const static std::unordered_map<char, SingleDoubleChoice>
        valid_two_character{
            {'(',
             {.m_two_character_token = "()",
              .m_single_token = TokenType::LPAREN,
              .m_double_token = TokenType::UNIT}},
            {'-',
             {.m_two_character_token = "->",
              .m_single_token = TokenType::MINUS,
              .m_double_token = TokenType::ARROW}},
            {'=',
             {.m_two_character_token = "==",
              .m_single_token = TokenType::EQUALS,
              .m_double_token = TokenType::EQ_EQ}},
            {'|',
             {.m_two_character_token = "||",
              .m_single_token = TokenType::PIPE,
              .m_double_token = TokenType::OR_OR}},
            {'!',
             {.m_two_character_token = "!=",
              .m_single_token = TokenType::BANG,
              .m_double_token = TokenType::BANG_EQ}},
            {'<',
             {.m_two_character_token = "<=",
              .m_single_token = TokenType::LESS,
              .m_double_token = TokenType::LESS_EQ}},
            {'>',
             {.m_two_character_token = ">=",
              .m_single_token = TokenType::GREATER,
              .m_double_token = TokenType::GREATER_EQ}},
            {'&',
             {.m_two_character_token = "&&",
              .m_single_token = TokenType::AND,
              .m_double_token = TokenType::AND_AND}},
        };
    // Do the slash as part of the comment since it coule be one of three
    // things: /, //, /*

    auto iter = valid_two_character.find(character);

    if (iter == valid_two_character.end()) {
      return {};
    }

    const SingleDoubleChoice &choice = iter->second;

    const auto start_position = get_current_loc();

    advance();
    // Cannot expect one of these at EOF
    expect_peek(character,
                "Invalid usage of Lexer::try_single_or_double_char_token, did "
                "not expect EOF");
    if (character == choice.m_two_character_token[1]) {
      advance();
      return make_token(start_position, choice.m_double_token);
    }
    return make_token(start_position, choice.m_single_token);
  }

  std::unique_ptr<Token> get_next_token() override {
    char character;
    while (peek(character)) {
      if (auto output = try_single_char_token(character)) {
        return output;
      }
      if (auto output = try_single_or_double_char_token(character)) {
        return output;
      }
      if (is_spacing_char(character)) {
        whitespace();
        continue;
      }
      if (is_line_terminator_char(character)) {
        line_break();
        continue;
      }
      if (is_digit(character)) {
        return int_literal();
      }
      if (is_slash(character)) {
        if (auto output = slash_or_comment()) {
          return output;
        }
        continue;
      }
      if (is_single_quote(character)) {
        return char_literal();
      }

      return identifier();
    }
    auto current_loc = get_current_loc();
    reset_eof();
    return make_token(current_loc, TokenType::EOF_TOKEN);
  }

  std::unique_ptr<Token> slash_or_comment() {
    char character;
    const auto location = get_current_loc();
    if (advance() != '/') {
      on_error("Invalid usage of Lexer::slash_or_comment, first character must "
               "be '/'");
    }
    if (peek(character)) {
      if (is_slash(character)) {
        comment();
        return {};
      }
      if (character == '*') {
        block_comment();
        return {};
      }
    }

    return make_token(location, TokenType::SLASH);
  }

  void comment() {
    // Expects already consumed previous SLASH
    char character;
    if (advance() != '/') {
      on_error("Invalid usage of Lexer::comment, second character must be '/' "
               "after '/'");
    }

    while (peek(character)) {
      if (is_line_terminator_char(character)) {
        line_break();
        return;
      }
      advance();
    }
  }

  void block_comment() {
    // Expects already consumed previous SLASH
    char character;
    if (advance() != '*') {
      on_error("Invalid usage of Lexer::comment, second character must be '*' "
               "after '/'");
    }
    while (peek(character)) {
      if (is_line_terminator_char(character)) {
        line_break();
        continue;
      }
      if (character == '*') {
        advance();
        if (peek(character) && is_slash(character)) {
          advance();
          return;
        }
        continue;
      }
      advance();
    }

    on_error("Did not see end of block comment at ", get_current_loc());
  }

  char expect_peek_advance(const std::function<bool(char)> &matches,
                           const char *calling_func) {
    char character;
    expect_peek(character,
                "Invalid usage of Lexer::char_literal, did not expect EOF");
    if (matches(character)) {
      return advance();
    }
    unexpected_character(character, calling_func);
  }

  std::unique_ptr<Token> char_literal() {
    const core::SourceLocation loc = get_current_loc();
    std::string lexeme;

    char character;
    lexeme += expect_peek_advance(is_single_quote, __FUNCTION__);

    // Could be 0-9, a-z, A-Z
    expect_peek(character,
                "Invalid usage of Lexer::char_literal, did not expect EOF");

    if (character == '\'') {
      // Premature end of quote
      unexpected_character(character, __FUNCTION__);
    }

    if (character == '\\') {
      // Escape sequence
      lexeme += advance();

      expect_peek(character,
                  "Invalid usage of Lexer::char_literal, did not expect EOF");
      if (character == 'x') {
        // Hex sequence
        lexeme += advance();
        lexeme += expect_peek_advance(is_hex_character, __FUNCTION__);
        lexeme += expect_peek_advance(is_hex_character, __FUNCTION__);
      } else if (is_escape_sequence_terminator(character)) {
        lexeme += advance();
      } else {
        unexpected_character(character, __FUNCTION__);
      }
    } else if (is_printable_char(character)) {
      lexeme += advance();
    } else {
      unexpected_character(character, __FUNCTION__);
    }

    lexeme += expect_peek_advance(is_single_quote, __FUNCTION__);

    return make_token(loc, TokenType::CHAR_LITERAL, lexeme);
  }

  std::unique_ptr<Token> int_literal() {
    const core::SourceLocation loc = get_current_loc();
    std::string lexeme;

    char character;
    expect_peek(character,
                "Invalid usage of Lexer::int_literal, did not expect EOF");

    // Next character determines if we can have multiple digits
    if (is_digit(character)) {
      lexeme += advance();
      if (character != '0') {
        while (peek(character)) {
          if (!is_digit(character)) {
            break;
          }
          lexeme += advance();
        }
      }
    } else {
      unexpected_character(character, __FUNCTION__);
    }

    return make_token(loc, TokenType::INT_LITERAL, lexeme);
  }

  std::unique_ptr<Token> single_char_token(TokenType type) {
    auto output = make_token(type);
    advance();
    return output;
  }

  [[nodiscard]] std::unique_ptr<Token>
  create_identifier(const core::SourceLocation &starting_loc,
                    const std::string &lexeme) const {
    if (lexeme.empty()) {
      on_error("Invalid usage of Lexer::create_identifier, did not find any "
               "characters to form the lexeme");
    }
    auto token_type = lookup_keyword(lexeme);
    if (token_type == TokenType::IDENTIFIER) {
      return make_token(starting_loc, token_type, lexeme);
    }
    return make_token(starting_loc, token_type);
  }

  std::unique_ptr<Token> identifier() {
    std::string lexeme;
    char character;
    core::SourceLocation starting_loc = get_current_loc();
    expect_peek(character, __FUNCTION__);

    if (!is_lower(character) && !is_upper(character) && character != '_') {
      on_error("Identifier must begin with ASCII character in [_a-z], not '",
               character, "'");
    }
    lexeme += advance();

    while (peek(character)) {
      if (is_lower(character) || is_upper(character) || character == '_' ||
          is_digit(character)) {
        lexeme += advance();
        continue;
      }
      break;
    }
    return create_identifier(starting_loc, lexeme);
  }

  [[nodiscard]] TokenType lookup_keyword(std::string_view lexeme) const {
    if (m_keywords.count(lexeme) == 1) {
      return m_keywords.at(lexeme);
    }
    return TokenType::IDENTIFIER;
  }

  void whitespace() {
    // Peek at next character, if whitespace, keep advancing until not.
    char character;
    while (peek(character)) {
      if (is_spacing_char(character)) {
        advance();
        continue;
      }
      return;
    }
  }

  bool peek(char &character) const {
    auto result = m_in_stream.peek();

    if (result == std::char_traits<char>::eof()) {
      return false;
    }
    character = static_cast<char>(result);
    return true;
  }

  void expect_peek(char &character, std::string_view additional_message) const {
    if (!peek(character)) {
      on_error("Lexer::expect_peek failed: ", additional_message);
    }
  }

  void line_break() {
    char character;
    while (peek(character)) {
      if (is_line_terminator_char(character)) {
        m_in_stream.get(character);
        m_line++;
        m_column = 0;
        continue;
      }
      return;
    }
  }

  char advance() {
    char character;
    if (!m_in_stream.get(character)) {
      on_error("Expected to be able to advance but saw EOF");
    }
    m_column++;
    return character;
  }

  void reset_eof() {
    m_in_stream.clear();
    m_line = 1;
    m_column = 0;
  }

  [[noreturn]]
  void unexpected_character(char character,
                            std::string_view function_name) const {
    on_error("Invalid usage of Lexer::", function_name,
             ", did not expect character ", character,
             " (value: ", static_cast<int>(character), ")");
  }

  [[nodiscard]] core::SourceLocation get_current_loc() const {
    return {.file_name = m_hint, .line = m_line, .column = m_column};
  }

  std::istream &m_in_stream;
  const std::unordered_map<std::string_view, TokenType> m_keywords;
  unsigned int m_line{1};
  unsigned int m_column{0};
  const char *m_hint;
};

//****************************************************************************
} // namespace
//****************************************************************************

std::unique_ptr<LexerInterface> make_lexer(std::istream &input,
                                           const char *hint) {
  return std::make_unique<Lexer>(input, hint);
}

//****************************************************************************
} // namespace bust
//****************************************************************************
