//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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
            const char *hint = "<input>")
      : m_in_stream(in_stream), m_keywords(keywords),
        m_current_loc(SourceLocation(hint)) {}

  std::unique_ptr<Token> get_next_token() {
    std::unique_ptr<Token> output = nullptr;
    char character;
    if (peek(character)) {
      switch (character) {
      case '(':
        return single_char_token(CommonTokenEnum::LEFT_PAREND, "(");
      case ')':
        return single_char_token(CommonTokenEnum::RIGHT_PAREND, ")");
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
      reset_eof();
      return std::make_unique<Token>(m_current_loc,
                                     CommonTokenEnum::EOF_TOKENTYPE, "EOF");
    } else {
      std::cerr << "Unexpected error!" << std::endl;
      return output;
    }
  }

  std::unique_ptr<Token> number() {
    // https://www.json.org/json-en.html
    SourceLocation loc(m_current_loc);
    std::string lexeme = "";

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
      const double value = std::stod(lexeme);
      return std::make_unique<TokenDouble>(loc, CommonTokenEnum::DOUBLE_LITERAL,
                                           lexeme, value);
    }

    if (has_minus) {
      if (lexeme.size() == 2 && lexeme.at(1) == '0') {
        // Very specific case that json.org supports but stoi doesn't
        return std::make_unique<TokenInt>(loc, CommonTokenEnum::INT_LITERAL,
                                          lexeme, 0);
      }
    }

    const int value = std::stoi(lexeme);
    return std::make_unique<TokenInt>(loc, CommonTokenEnum::INT_LITERAL, lexeme,
                                      value);
  }

  std::unique_ptr<Token> single_char_token(TokenType type,
                                           const std::string &lexeme) {
    std::unique_ptr<Token> output =
        std::make_unique<Token>(m_current_loc, type, lexeme);
    advance();
    return output;
  }

  std::unique_ptr<Token> string() {
    // https://www.json.org/json-en.html
    std::string value = "";
    char character;
    SourceLocation starting_loc = SourceLocation(m_current_loc);

    expect_peek(character,
                "Invalid usage of CoreLexer::string, did not expect EOF");

    // Don't need the " character
    if (advance() != '"') {
      std::string exception_message =
          "Invalid usage of CoreLexer::string, first character must be '\"'";
      throw LexerException(exception_message, m_current_loc);
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
      std::string exception_message =
          "Invalid usage of CoreLexer::string, did not expect EOF before final "
          "double quote (\")";
      throw LexerException(exception_message, m_current_loc);
    }

    std::string lexeme = std::string("\\\"" + value + "\\\"");
    return std::make_unique<TokenString>(
        starting_loc, CommonTokenEnum::STRING_LITERAL, lexeme, value);
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

    std::string exception_message =
        "Invalid usage of CoreLexer::escaped_character, cannot escape "
        "character ";
    exception_message += character;
    exception_message += " (value: " + std::to_string(int(character)) + ").";
    throw LexerException(exception_message, m_current_loc);
  }

  std::unique_ptr<Token> identifier() {
    std::string lexeme = "";
    char character;
    SourceLocation starting_loc = SourceLocation(m_current_loc);
    while (peek(character)) {
      switch (character) {
      case '(':
      case ')':
      case ';':
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        // TODO: determine if literal or identifier
        return std::make_unique<Token>(starting_loc, lookup_keyword(lexeme),
                                       lexeme);
      default:
        lexeme += advance();
        break;
      }
    }
    if (lexeme == "") {
      std::string exception_message =
          "Invalid usage of CoreLexer::identifier, did not find any characters "
          "to form the lexeme";
      throw LexerException(exception_message, m_current_loc);
    }
    if (lexeme == "true" || lexeme == "false") {
      bool value;
      std::istringstream(lexeme) >> std::boolalpha >> value;
      return std::make_unique<TokenBool>(
          starting_loc, CommonTokenEnum::BOOL_LITERAL, lexeme, value);
    }

    return std::make_unique<Token>(starting_loc, lookup_keyword(lexeme),
                                   lexeme);
  }

  TokenType lookup_keyword(std::string_view lexeme) const {
    if (m_keywords.count(lexeme) == 1) {
      return m_keywords.at(lexeme);
    }
    return CommonTokenEnum::IDENTIFIER;
  }

  void comment() {
    // Peek at next character, if whitespace, keep advancing until not.
    char character;
    if (advance() != ';') {
      std::string exception_message =
          "Invalid usage of CoreLexer::comment, first character must be ';'";
      throw LexerException(exception_message, m_current_loc);
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
    std::stringstream exception_message;
    exception_message << "Did not see end of block comment at ";
    m_current_loc.dump(exception_message);
    throw LexerException(exception_message.str(), m_current_loc);
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

  void expect_peek(char &character, const std::string &additional_message) {
    if (!peek(character)) {
      std::string exception_message =
          "CoreLexer::expect_peek failed: " + additional_message;
      throw LexerException(exception_message, m_current_loc);
    }
  }

  void line_break() {
    char character;
    while (peek(character)) {
      switch (character) {
      case '\r':
      case '\n':
        m_in_stream.get(character);
        m_current_loc.m_line++;
        m_current_loc.m_column = 0;
        break;
      default:
        return;
      }
    }
  }

  char advance() {
    char character;
    if (!m_in_stream.get(character)) {
      std::stringstream exception_message;
      exception_message << "Expected to be able to advance but saw EOF";
      throw LexerException(exception_message.str(), m_current_loc);
    }
    m_current_loc.m_column++;
    return character;
  }

  void reset_eof() {
    m_in_stream.clear();
    m_current_loc.m_line = 1;
    m_current_loc.m_column = 0;
  }

  void unexpected_character(char character,
                            const std::string &function_name) const {
    std::string exception_message =
        "Invalid usage of CoreLexer::" + function_name +
        ", did not expect "
        "character ";
    exception_message = exception_message + character +
                        " (value: " + std::to_string(int(character)) + ")";
    throw LexerException(exception_message, m_current_loc);
  }

  std::istream &m_in_stream;
  const std::unordered_map<std::string_view, TokenType> m_keywords;
  SourceLocation m_current_loc;
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
