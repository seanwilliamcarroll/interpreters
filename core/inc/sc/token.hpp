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
const sc::TokenType PLUS = 5;
const sc::TokenType MINUS = 6;
const sc::TokenType TIMES = 7;
const sc::TokenType DIV = 8;
const sc::TokenType EQ = 9;
const sc::TokenType EQEQ = 10;
const sc::TokenType LT = 11;
const sc::TokenType LTEQ = 12;
const sc::TokenType GT = 13;
const sc::TokenType GTEQ = 14;
const sc::TokenType XOR = 15;
const sc::TokenType TILDE = 16;
const sc::TokenType NOT = 17;
const sc::TokenType NOTEQ = 18;
const sc::TokenType DOT = 19;
const sc::TokenType INT_LITERAL = 20;
const sc::TokenType STRING_LITERAL = 21;
const sc::TokenType DOUBLE_LITERAL = 22;
const sc::TokenType BOOL_LITERAL = 23;
} // namespace token_type

struct Token {

  Token(const SourceLocation &loc, TokenType type, const std::string &lexeme);

  virtual ~Token();

  virtual std::ostream &dump(std::ostream &out);

  virtual bool is_same_type_as(Token const &other) const;

  virtual bool is_same_lexeme_as(Token const &other) const;

  virtual bool is_same_type_lexeme_as(Token const &other) const;

  bool operator==(Token const &other) const;

  const SourceLocation m_loc;
  const TokenType m_type;
  const std::string m_lexeme;
};

template <typename T> struct TokenOf : public Token {

  TokenOf(const SourceLocation &loc, TokenType type, const std::string &lexeme,
          const T &value);

  virtual ~TokenOf();

  bool operator==(TokenOf const &other) const;

  virtual std::ostream &dump(std::ostream &out) override;

  const T m_value;
};

using TokenString = TokenOf<std::string>;
using TokenInt = TokenOf<int>;
using TokenDouble = TokenOf<double>;
using TokenBool = TokenOf<bool>;

//****************************************************************************
} // namespace sc
//****************************************************************************
