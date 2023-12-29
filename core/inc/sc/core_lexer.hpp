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

#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

class CoreLexer : public LexerInterface {
public:
  CoreLexer(std::istream &in_stream, const KeywordsMap &keywords,
            const std::string &file_name = "<input>");

  virtual std::unique_ptr<Token> get_next_token() override;

protected:
  void comment();
  void whitespace();
  void line_break();
  char advance();
  void block_comment();
  std::unique_ptr<Token> single_char_token(TokenType type,
                                           const std::string &lexeme);
  std::unique_ptr<Token> comparison_operator();
  std::unique_ptr<Token> identifier();
  std::unique_ptr<Token> string();
  std::string escaped_character();
  std::unique_ptr<Token> minus_or_number();
  std::unique_ptr<Token> number(const SourceLocation &original_loc,
                                const std::string &prefix = "");
  TokenType lookup_keyword(const std::string &lexeme);
  bool peek(char &character);
  void expect_peek(char &character, const std::string &additional_message);
  void reset_eof();
  void unexpected_character(char character, const std::string &function_name);
};

//****************************************************************************
} // namespace sc
//****************************************************************************
