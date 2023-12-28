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
    : m_in_stream(in_stream), m_keywords(keywords),
      m_current_loc(SourceLocation(file_name)) {}

std::unique_ptr<Token> CoreLexer::getNextToken() {
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
      return getNextToken();
    case ' ':
    case '\t':
      // Whitespace
      whitespace();
      return getNextToken();
    case '\r':
    case '\n':
      // Line break
      line_break();
      return getNextToken();
    case '+':
      return single_char_token(token_type::PLUS, "+");
    case '-':
      return single_char_token(token_type::MINUS, "-");
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
    default:
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
      std::string exception_message =
          "Invalid usage of CoreLexer::comparison_operator, did not expect "
          "character ";
      exception_message = exception_message + std::to_string(character) +
                          " (value: " + std::to_string(int(character)) + ")";
      throw LexerException(exception_message);
      break;
    }
  } else {
    std::string exception_message =
        "Invalid usage of CoreLexer::comparison_operator, did not expect EOF";
    throw LexerException(exception_message);
  }
  return std::make_unique<Token>(starting_loc, type, lexeme);
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
    throw LexerException(exception_message);
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

  advance();

  if (peek(character)) {
    if (character == '-') {
      advance();
      block_comment();
      return;
    }
  } else {
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
      if (peek(character)) {
        if (character == ';') {
          advance();
          return;
        }
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
  throw LexerException(exception_message.str());
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
    character = '\0';
  }
  m_current_loc.m_column++;
  return character;
}

void CoreLexer::reset_eof() {
  m_in_stream.clear();
  m_current_loc.m_line = 1;
  m_current_loc.m_column = 0;
}

//****************************************************************************
} // namespace sc
//****************************************************************************
