//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : CoreLexer class source file
//*
//*
//****************************************************************************

#include <cctype>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include <sc/exceptions.hpp>      // For exceptions classes
#include <sc/lexer_interface.hpp> // For forward decls
#include <sc/sc.hpp>              // For forward decls
#include <sc/token.hpp>           // For Token classes

//****************************************************************************
namespace sc {
namespace {
//****************************************************************************

struct CoreLexer : LexerInterface {

  CoreLexer(std::istream &in_stream, std::initializer_list<Keyword> keywords,
            const char *hint)
      : m_in_stream(in_stream), m_keywords(keywords), m_line(1), m_column(0),
        m_hint(hint) {}

  auto make_token(TokenType t) const {
    return std::make_unique<Token>(get_current_loc(), t);
  }

  auto make_token(const SourceLocation &l, TokenType t) const {
    return std::make_unique<Token>(l, t);
  }

  template <class value>
  auto make_token(const SourceLocation &l, TokenType t, const value &v) const {
    return std::make_unique<TokenOf<value>>(l, t, v);
  }

  template <class... args> [[noreturn]] void on_error(args &&...a) const {

    std::ostringstream o; // Local stringstream

    (o << ... << a); // Fold operator <<

    throw CompilerException(o.str(), get_current_loc());
  }

  std::unique_ptr<Token> get_next_token() {
    char character;
    if (peek(character)) {
      switch (character) {
      case '(':
        return single_char_token(Token::LEFT_PAREND);
      case ')':
        return single_char_token(Token::RIGHT_PAREND);
      case ';':
        // Start of end of line comment or block comment
        comment();
        return get_next_token();
      case ' ':
      case '\t':
        // Whitespace
        whitespace();
        return get_next_token();
      case '\r':
      case '\n':
        // Line break
        line_break();
        return get_next_token();
      case '-':
        return number();
      case '"':
        return string();
      default:
        if (std::isdigit(character)) {
          return number();
        }
        return identifier();
      }
    } else if (m_in_stream.eof()) {
      auto current_loc = get_current_loc();
      reset_eof();
      return make_token(current_loc, Token::EOF_TOKENTYPE);
    } else {
      std::cerr << "Unexpected error!" << std::endl;
      return {};
    }
  }

  std::unique_ptr<Token> number() {
    // https://www.json.org/json-en.html
    const SourceLocation loc = get_current_loc();
    std::string lexeme;

    bool is_double = false;
    bool has_minus = false;

    char character;
    // First check for optional '-' character
    expect_peek(character,
                "Invalid usage of CoreLexer::number, did not expect EOF");
    if (character == '-') {
      lexeme += advance();
      has_minus = true;
    }

    // Next character determines if we can have multiple digits
    expect_peek(character,
                "Invalid usage of CoreLexer::number, did not expect EOF");
    if (std::isdigit(character)) {
      lexeme += advance();
      if (character != '0') {
        while (peek(character)) {
          if (!std::isdigit(character)) {
            break;
          }
          lexeme += advance();
        }
      }
    } else if (std::isspace(character)) {
      return create_identifier(loc, lexeme);
    } else {
      unexpected_character(character, __FUNCTION__);
    }

    // Check if fractional
    if (peek(character) && character == '.') {
      is_double = true;
      lexeme += advance();
      expect_peek(character,
                  "Invalid usage of CoreLexer::number, did not expect EOF");
      if (!std::isdigit(character)) {
        unexpected_character(character, __FUNCTION__);
      }
      while (peek(character)) {
        if (!std::isdigit(character)) {
          break;
        }
        lexeme += advance();
      }
    }

    // Check if exponent
    if (peek(character) && (character == 'e' || character == 'E')) {
      is_double = true;
      lexeme += advance();
      expect_peek(character,
                  "Invalid usage of CoreLexer::number, did not expect EOF");
      if (!(std::isdigit(character) || (character == '-') ||
            (character == '+'))) {
        unexpected_character(character, __FUNCTION__);
      }
      if (character == '-' || character == '+') {
        lexeme += advance();
      }
      while (peek(character)) {
        if (!std::isdigit(character)) {
          break;
        }
        lexeme += advance();
      }
    }

    // Create and return token
    if (is_double) {
      return make_token(loc, Token::DOUBLE_LITERAL, std::stod(lexeme));
    }

    if (has_minus) {
      if (lexeme.size() == 2 && lexeme.at(1) == '0') {
        // Very specific case that json.org supports but stoi doesn't
        return make_token(loc, Token::INT_LITERAL, 0);
      }
    }

    return make_token(loc, Token::INT_LITERAL, std::stoi(lexeme));
  }

  std::unique_ptr<Token> single_char_token(TokenType type) {
    auto output = make_token(type);
    advance();
    return output;
  }

  std::unique_ptr<Token> string() {
    // https://www.json.org/json-en.html
    std::string value;
    char character;
    const SourceLocation starting_loc = get_current_loc();

    expect_peek(character,
                "Invalid usage of CoreLexer::string, did not expect EOF");

    // Don't need the " character
    if (advance() != '"') {
      on_error(
          "Invalid usage of CoreLexer::string, first character must be '\"'");
    }

    bool last_char_double_quote = false;
    while (peek(character) && !last_char_double_quote) {
      last_char_double_quote = false;
      switch (character) {
      case '\\':
        advance();
        value += escaped_character();
        break;
      case '"':
        // Don't need the " character
        advance();
        last_char_double_quote = true;
        break;
      default:
        value += advance();
        break;
      }
    }
    if (!last_char_double_quote) {
      on_error("Invalid usage of CoreLexer::string, did not expect EOF before"
               " final double quote (\")");
    }

    return make_token(starting_loc, Token::STRING_LITERAL, value);
  }

  std::string escaped_character() {
    char character;
    expect_peek(character, "Invalid usage of CoreLexer::string, did not expect "
                           "EOF before final double quote (\")");
    switch (character) {
    case '"':
    case '\\':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
      return std::string({'\\', advance()});
      // case 'u': // Don't need hex value for the moment
    }

    on_error("Invalid usage of CoreLexer::escaped_character, cannot escape "
             "character ",
             character, " (value: ", static_cast<int>(character), ")");
  }

  std::unique_ptr<Token> create_identifier(const SourceLocation &starting_loc,
                                           const std::string &lexeme) {
    if (lexeme.empty()) {
      on_error(
          "Invalid usage of CoreLexer::create_identifier, did not find any "
          "characters to form the lexeme");
    }
    if (lexeme == "true") {
      return make_token(starting_loc, Token::BOOL_LITERAL, true);
    }
    if (lexeme == "false") {
      return make_token(starting_loc, Token::BOOL_LITERAL, false);
    }
    auto token_type = lookup_keyword(lexeme);
    if (token_type == Token::IDENTIFIER) {
      return make_token(starting_loc, token_type, lexeme);
    } else {
      return make_token(starting_loc, token_type);
    }
  }

  std::unique_ptr<Token> identifier() {
    std::string lexeme;
    char character;
    SourceLocation starting_loc = get_current_loc();
    bool is_end = false;
    expect_peek(character, __FUNCTION__);
    while (peek(character) && !is_end) {
      switch (character) {
      case '(':
      case ')':
      case ';':
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        is_end = true;
        break;
      default:
        lexeme += advance();
        break;
      }
    }
    return create_identifier(starting_loc, lexeme);
  }

  TokenType lookup_keyword(std::string_view lexeme) const {
    if (m_keywords.count(lexeme) == 1) {
      return m_keywords.at(lexeme);
    }
    return Token::IDENTIFIER;
  }

  void comment() {
    // Peek at next character, if whitespace, keep advancing until not.
    char character;
    if (advance() != ';') {
      on_error(
          "Invalid usage of CoreLexer::comment, first character must be ';'");
    }

    if (!peek(character)) {
      return;
    }
    if (character == '-') {
      advance();
      block_comment();
      return;
    }

    while (peek(character)) {
      switch (character) {
      case '\r':
      case '\n':
        line_break();
        return;
      default:
        advance();
      }
    }
  }

  void block_comment() {
    // Keep throwing away characters until we see the end of a block comment
    // If we hit EOF, throw an exception
    char character;
    while (peek(character)) {
      switch (character) {
      case '-':
        advance();
        if (peek(character) && character == ';') {
          advance();
          return;
        }
        break;
      case '\r':
      case '\n':
        line_break();
        break;
      default:
        advance();
        break;
      }
    }

    on_error("Did not see end of block comment at ", get_current_loc());
  }

  void whitespace() {
    // Peek at next character, if whitespace, keep advancing until not.
    char character;
    while (peek(character)) {
      switch (character) {
      case ' ':
      case '\t':
        advance();
        break;
      default:
        return;
      }
    }
  }

  bool peek(char &character) {
    char temp_character = m_in_stream.peek();

    if (temp_character == EOF) {
      m_in_stream.get(character);
      return false;
    }
    character = temp_character;
    return true;
  }

  void expect_peek(char &character, std::string_view additional_message) {
    if (!peek(character)) {
      on_error("CoreLexer::expect_peek failed: ", additional_message);
    }
  }

  void line_break() {
    char character;
    while (peek(character)) {
      switch (character) {
      case '\r':
      case '\n':
        m_in_stream.get(character);
        m_line++;
        m_column = 0;
        break;
      default:
        return;
      }
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

  void unexpected_character(char character,
                            std::string_view function_name) const {
    on_error("Invalid usage of CoreLexer::", function_name,
             ", did not expect character ", character,
             " (value: ", static_cast<int>(character), ")");
  }

  SourceLocation get_current_loc() const {
    return SourceLocation(m_hint, m_line, m_column);
  }

  std::istream &m_in_stream;
  const std::unordered_map<std::string_view, TokenType> m_keywords;
  unsigned int m_line;
  unsigned int m_column;
  const char *m_hint;
};

//****************************************************************************
} // namespace
//****************************************************************************

std::unique_ptr<LexerInterface>
make_lexer(std::istream &in_stream, std::initializer_list<Keyword> keywords,
           const char *hint) {
  return std::make_unique<CoreLexer>(in_stream, keywords, hint);
}

//****************************************************************************
} // namespace sc
//****************************************************************************
