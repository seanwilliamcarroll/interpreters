//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Lexer
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <sc/sc.hpp>
#include <sc/core_lexer.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

namespace blip_token_types {
  const sc::TokenType IF = 5;
  const sc::TokenType WHILE = 6;
  const sc::TokenType SET = 7;
  const sc::TokenType BEGIN = 8;
  const sc::TokenType PLUS = 9;
  const sc::TokenType MINUS = 10;
  const sc::TokenType TIMES = 11;
  const sc::TokenType DIV = 12;
  const sc::TokenType EQ = 13;
  const sc::TokenType LT = 14;
  const sc::TokenType GT = 15;
  const sc::TokenType PRINT = 16;
}


  
class Lexer : public sc::CoreLexer {
public:
  Lexer(std::istream& in_stream, const std::string& file_name);

private:
  sc::KeywordsMap construct_keywords_map();
  
};
  
//****************************************************************************
} // namespace blip
//****************************************************************************
