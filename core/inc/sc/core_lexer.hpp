//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Core Lexer Implementation
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <iostream>
#include <memory>

#include <sc/sc.hpp>
#include <sc/token.hpp>
#include <sc/lexer_interface.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************


class CoreLexer : public LexerInterface {
public:

  CoreLexer(std::istream& in_stream, const KeywordsMap& keywords, const std::string& file_name = "<input>");

  virtual std::unique_ptr<Token> getNextToken() override;

protected:
  void comment();
  void whitespace();
  void line_break();
  char advance();
  void block_comment();
  std::unique_ptr<Token> identifier();
  TokenType lookup_keyword(const std::string& lexeme);
  bool peek(char& character);
  void reset_eof();
  
  
  
private:
  std::istream& m_in_stream;
  const KeywordsMap m_keywords;
  SourceLocation m_current_loc;
};

  
//****************************************************************************
} // namespace sc
//****************************************************************************
