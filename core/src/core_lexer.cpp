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
#include <string>
#include <memory>

#include <sc/sc.hpp>                                   // For forward decls
#include <sc/token.hpp>                                // For Token classes
#include <sc/core_lexer.hpp>                           // For CoreLexer class

//****************************************************************************
namespace sc {
//****************************************************************************

CoreLexer::CoreLexer(std::istream& in_stream, const KeywordsMap& keywords, const std::string& file_name)
  : m_in_stream(in_stream), m_keywords(keywords), m_current_loc(SourceLocation(file_name)) {}

std::unique_ptr<Token> CoreLexer::getNextToken() {
  std::unique_ptr<Token> output = nullptr;
  char character;
  if (peek(character)) {
    switch(character) {
    case '(':
      output = std::make_unique<Token>(m_current_loc, token_type::LEFT_PAREND, "(");
      advance();
      return output;
      break;
    case ')':
      output = std::make_unique<Token>(m_current_loc, token_type::RIGHT_PAREND, ")");
      advance();
      return output;
      break;
    case ';':
      // Start of end of line comment or block comment
      comment();
      return getNextToken();
      break;
    case ' ':
    case '\t':
      // Whitespace
      whitespace();
      return getNextToken();
      break;
    case '\r':
    case '\n':
      // Line break
      line_break();
      return getNextToken();
      break;
    default:
      return identifier();
      break;
    }
  } else if (m_in_stream.eof()) {
    reset_eof();
    return std::make_unique<Token>(m_current_loc, token_type::EOF_TOKENTYPE, "EOF");
  } else {
    std::cerr << "Unexpected error!" << std::endl;
    return output;
  }
}

std::unique_ptr<Token>  CoreLexer::identifier(){
  std::string lexeme = "";
  char character;
  SourceLocation starting_loc = SourceLocation(m_current_loc);
  while (peek(character)) {
    switch(character){
    case '(':
    case ')':
    case ';':
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      // TODO: determine if literal or identifier
      return std::make_unique<Token>(starting_loc, lookup_keyword(lexeme), lexeme);
    default:
      lexeme += advance();
      break;
    }
  }
  if (lexeme == ""){
    // TODO
    // throw std::exception
  }
  return std::make_unique<Token>(starting_loc, lookup_keyword(lexeme), lexeme);
}

TokenType CoreLexer::lookup_keyword(const std::string& lexeme){
  if (m_keywords.count(lexeme) == 1) {
    return m_keywords.at(lexeme);
  }
  return token_type::IDENTIFIER;
}

  
void CoreLexer::comment(){
  // Peek at next character, if whitespace, keep advancing until not.
  char character;

  advance();
  
  if (peek(character)){
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

void CoreLexer::block_comment(){
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
  // TODO
  // throw std::exception;
}

  
void CoreLexer::whitespace(){
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

bool CoreLexer::peek(char& character) {
  char temp_character = m_in_stream.peek();

  if (temp_character == EOF) {
    m_in_stream.get(character);
    return false;
  }
  character = temp_character;
  return true;
}
  
void CoreLexer::line_break(){
  char character;
  while (peek(character)) {
    switch (character) {
    case '\r':
    case '\n':
      if (!m_in_stream.get(character)){
        character = '\0';
      }
      m_current_loc.m_line++;
      m_current_loc.m_column = 0;
      break;
    default:
      return;
    }
  }
}

char CoreLexer::advance(){
  char character;
  if (!m_in_stream.get(character)){
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
