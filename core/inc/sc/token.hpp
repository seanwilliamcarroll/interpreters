//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Token and TokenOf<T> classes
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <iostream>

#include <sc/sc.hpp>
#include <sc/source_location.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

namespace token_type {
const TokenType INVALID = 0;
const TokenType EOF_TOKENTYPE = 1;
const TokenType LEFT_PAREND = 2;
const TokenType RIGHT_PAREND = 3;
const TokenType IDENTIFIER = 4;
} // namespace token_type

struct Token {

  Token(const SourceLocation &loc, TokenType type, const std::string &lexeme)
      : m_loc(loc), m_type(type), m_lexeme(lexeme) {}

  virtual ~Token() {}

  virtual std::ostream &dump(std::ostream &out);

  const SourceLocation m_loc;
  const TokenType m_type;
  const std::string m_lexeme;
};

template <typename T> struct TokenOf : public Token {

  TokenOf(const SourceLocation &loc, TokenType type, const std::string &lexeme,
          const T &value)
      : Token(loc, type, lexeme), m_value(value) {}

  virtual ~TokenOf() {}

  virtual std::ostream &dump(std::ostream &out) override;

  const T m_value;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
