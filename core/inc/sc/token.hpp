//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
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

#include <iosfwd>
#include <string>

#include <sc/sc.hpp>
#include <sc/source_location.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

class Token {
public:
  enum CommonTokenType : TokenType {
    EOF_TOKENTYPE = 0,
    LEFT_PAREND,
    RIGHT_PAREND,
    IDENTIFIER,
    INT_LITERAL,
    STRING_LITERAL,
    DOUBLE_LITERAL,
    BOOL_LITERAL,

    START_TOKEN = EOF_TOKENTYPE,
    END_TOKEN = BOOL_LITERAL
  };

  Token(const SourceLocation &, TokenType);

  virtual ~Token() = default;

  virtual std::ostream &dump(std::ostream &) const;

  const auto &get_location() const { return m_loc; }

  auto get_token_type() const { return m_type; }

private:
  const SourceLocation m_loc;
  const TokenType m_type;
};

template <typename T> class TokenOf : public Token {
public:
  TokenOf(const SourceLocation &, TokenType, const T &value);

  virtual std::ostream &dump(std::ostream &) const;

  const T &get_value() const { return m_value; }

private:
  const T m_value;
};

using TokenIdentifier = TokenOf<std::string>;
using TokenString = TokenOf<std::string>;
using TokenInt = TokenOf<int>;
using TokenDouble = TokenOf<double>;
using TokenBool = TokenOf<bool>;

std::ostream &operator<<(std::ostream &, const Token &);

//****************************************************************************
} // namespace sc
//****************************************************************************
