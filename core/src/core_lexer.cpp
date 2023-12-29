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

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <sc/core_lexer.hpp> // For CoreLexer class
#include <sc/exceptions.hpp> // For exceptions classes
#include <sc/sc.hpp>         // For forward decls
#include <sc/token.hpp>      // For Token classes

//****************************************************************************
namespace sc {
//****************************************************************************

CoreLexer::CoreLexer(std::istream &in_stream, const KeywordsMap &keywords,
                     const std::string &file_name)
    : LexerInterface(in_stream, keywords, file_name) {}

bool is_numeric(char character) { return character >= '0' && character <= '9'; }

bool is_alpha(char character) {
  return (character >= 'a' && character <= 'z') ||
         (character >= 'A' && character <= 'Z');
}

bool is_alphanumeric(char character) {
  return is_alpha(character) || is_numeric(character);
}

std::unique_ptr<Token> CoreLexer::get_next_token() {
  std::unique_ptr<Token> output = nullptr;
  char character;
  if (peek(character)) {
    switch (character) {
    case '(':
      return single_char_token(token_type::LEFT_PAREND, "(");
    case ')':
      return single_char_token(token_type::RIGHT_PAREND, ")");
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
    case '+':
      return single_char_token(token_type::PLUS, "+");
    case '-':
      return minus_or_number();
    case '*':
      return single_char_token(token_type::TIMES, "*");
    case '/':
      return single_char_token(token_type::DIV, "/");
    case '^':
      return single_char_token(token_type::XOR, "^");
    case '~':
      return single_char_token(token_type::TILDE, "~");
    case '.':
      return single_char_token(token_type::DOT, ".");
    case '=':
    case '>':
    case '<':
    case '!':
      return comparison_operator();
    case '"':
      return string();
    default:
      if (is_numeric(character)) {
        return number(m_current_loc);
      }
      return identifier();
    }
  } else if (m_in_stream.eof()) {
    reset_eof();
    return std::make_unique<Token>(m_current_loc, token_type::EOF_TOKENTYPE,
                                   "EOF");
  } else {
    std::cerr << "Unexpected error!" << std::endl;
    return output;
  }
}

std::unique_ptr<Token> CoreLexer::number(const SourceLocation &original_loc,
                                         const std::string &prefix) {
  // https://www.json.org/json-en.html
  SourceLocation loc(original_loc);
  std::string lexeme(prefix);

  bool is_double = false;

  // First character determines if we can have multiple digits
  char character;
  expect_peek(character,
              "Invalid usage of CoreLexer::number, did not expect EOF");
  if (is_numeric(character)) {
    lexeme += advance();
    if (character != '0') {
      while (peek(character)) {
        if (!is_numeric(character)) {
          break;
        }
        lexeme += advance();
      }
    }
  }

  // Check if fractional
  if (peek(character) && character == '.') {
    is_double = true;
    lexeme += advance();
    expect_peek(character,
                "Invalid usage of CoreLexer::number, did not expect EOF");
    if (!is_numeric(character)) {
      unexpected_character(character, __FUNCTION__);
    }
    while (peek(character)) {
      if (!is_numeric(character)) {
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
    if (!(is_numeric(character) || (character == '-') || (character == '+'))) {
      unexpected_character(character, __FUNCTION__);
    }
    if (character == '-' || character == '+') {
      lexeme += advance();
    }
    while (peek(character)) {
      if (!is_numeric(character)) {
        break;
      }
      lexeme += advance();
    }
  }

  // Create and return token
  if (is_double) {
    double value = std::stod(lexeme);
    return std::make_unique<TokenDouble>(loc, token_type::DOUBLE_LITERAL,
                                         lexeme, value);
  }
  int value = std::stoi(lexeme);
  return std::make_unique<TokenInt>(loc, token_type::INT_LITERAL, lexeme,
                                    value);
}

std::unique_ptr<Token> CoreLexer::minus_or_number() {
  SourceLocation current_loc = SourceLocation(m_current_loc);
  std::unique_ptr<Token> output =
      std::make_unique<Token>(m_current_loc, token_type::MINUS, "-");
  char character;
  character = advance();
  if (character != '-') {
    unexpected_character(character, __FUNCTION__);
  }

  if (peek(character) && is_numeric(character)) {
    return number(current_loc, "-");
  }
  return output;
}

std::unique_ptr<Token> CoreLexer::single_char_token(TokenType type,
                                                    const std::string &lexeme) {
  std::unique_ptr<Token> output =
      std::make_unique<Token>(m_current_loc, type, lexeme);
  advance();
  return output;
}

std::unique_ptr<Token> CoreLexer::comparison_operator() {
  std::string lexeme = "";
  char character;
  SourceLocation starting_loc = SourceLocation(m_current_loc);
  TokenType type = token_type::INVALID;
  if (peek(character)) {
    switch (character) {
    case '=':
      lexeme += advance();
      if (peek(character) && character == '=') {
        lexeme += advance();
        type = token_type::EQEQ;
      } else {
        type = token_type::EQ;
      }
      break;
    case '<':
      lexeme += advance();
      if (peek(character) && character == '=') {
        lexeme += advance();
        type = token_type::LTEQ;
      } else {
        type = token_type::LT;
      }
      break;
    case '>':
      lexeme += advance();
      if (peek(character) && character == '=') {
        lexeme += advance();
        type = token_type::GTEQ;
      } else {
        type = token_type::GT;
      }
      break;
    case '!':
      lexeme += advance();
      if (peek(character) && character == '=') {
        lexeme += advance();
        type = token_type::NOTEQ;
      } else {
        type = token_type::NOT;
      }
      break;
    default:
      unexpected_character(character, __FUNCTION__);
      break;
    }
  } else {
    std::string exception_message =
        "Invalid usage of CoreLexer::comparison_operator, did not expect EOF";
    throw LexerException(exception_message, m_current_loc);
  }
  return std::make_unique<Token>(starting_loc, type, lexeme);
}

std::unique_ptr<Token> CoreLexer::string() {
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
  return std::make_unique<TokenString>(starting_loc, token_type::STRING_LITERAL,
                                       lexeme, value);
}

std::string CoreLexer::escaped_character() {
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
      "Invalid usage of CoreLexer::escaped_character, cannot escape character ";
  exception_message += character;
  exception_message += " (value: " + std::to_string(int(character)) + ").";
  throw LexerException(exception_message, m_current_loc);
}

std::unique_ptr<Token> CoreLexer::identifier() {
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
    return std::make_unique<TokenBool>(starting_loc, token_type::BOOL_LITERAL,
                                       lexeme, value);
  }

  return std::make_unique<Token>(starting_loc, lookup_keyword(lexeme), lexeme);
}

TokenType CoreLexer::lookup_keyword(const std::string &lexeme) {
  if (m_keywords.count(lexeme) == 1) {
    return m_keywords.at(lexeme);
  }
  return token_type::IDENTIFIER;
}

void CoreLexer::comment() {
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

void CoreLexer::block_comment() {
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

void CoreLexer::whitespace() {
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

bool CoreLexer::peek(char &character) {
  char temp_character = m_in_stream.peek();

  if (temp_character == EOF) {
    m_in_stream.get(character);
    return false;
  }
  character = temp_character;
  return true;
}

void CoreLexer::expect_peek(char &character,
                            const std::string &additional_message) {
  if (!peek(character)) {
    std::string exception_message =
        "CoreLexer::expect_peek failed: " + additional_message;
    throw LexerException(exception_message, m_current_loc);
  }
}

void CoreLexer::line_break() {
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

char CoreLexer::advance() {
  char character;
  if (!m_in_stream.get(character)) {
    std::stringstream exception_message;
    exception_message << "Expected to be able to advance but saw EOF";
    throw LexerException(exception_message.str(), m_current_loc);
  }
  m_current_loc.m_column++;
  return character;
}

void CoreLexer::reset_eof() {
  m_in_stream.clear();
  m_current_loc.m_line = 1;
  m_current_loc.m_column = 0;
}

void CoreLexer::unexpected_character(char character,
                                     const std::string &function_name) {
  std::string exception_message =
      "Invalid usage of CoreLexer::" + function_name +
      ", did not expect "
      "character ";
  exception_message = exception_message + character +
                      " (value: " + std::to_string(int(character)) + ")";
  throw LexerException(exception_message, m_current_loc);
}

//****************************************************************************
} // namespace sc
//****************************************************************************
