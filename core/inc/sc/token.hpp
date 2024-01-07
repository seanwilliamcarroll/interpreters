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

enum CommonTokenType : TokenType {
  INVALID_START = 0,

  EOF_TOKENTYPE,
  LEFT_PAREND,
  RIGHT_PAREND,
  IDENTIFIER,
  INT_LITERAL,
  STRING_LITERAL,
  DOUBLE_LITERAL,
  BOOL_LITERAL,

  INVALID_END
};

struct CommonTokenEnum {
  using enum CommonTokenType;
};

struct Token {

  Token(const SourceLocation &, TokenType, const std::string &lexeme);

  virtual ~Token();

  virtual std::ostream &dump(std::ostream &);

  virtual bool is_same_type_as(Token const &) const;

  virtual bool is_same_lexeme_as(Token const &) const;

  virtual bool is_same_type_lexeme_as(Token const &) const;

  bool operator==(Token const &) const;

  const SourceLocation m_loc;
  const TokenType m_type;
  const std::string m_lexeme;
};

template <typename T> struct TokenOf : public Token {

  TokenOf(const SourceLocation &, TokenType, const std::string &lexeme,
          const T &value);

  virtual ~TokenOf();

  bool operator==(TokenOf const &) const;

  virtual std::ostream &dump(std::ostream &) override;

  const T m_value;
};

using TokenString = TokenOf<std::string>;
using TokenInt = TokenOf<int>;
using TokenDouble = TokenOf<double>;
using TokenBool = TokenOf<bool>;

//****************************************************************************
} // namespace sc
//****************************************************************************
