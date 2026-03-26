//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Token, TokenOf classes source file
//*
//*
//****************************************************************************

#include <iostream>
#include <string>

#include "sc/exceptions.hpp" // For UnknownTokenTypeException classes
#include "sc/sc.hpp"         // For forward decls
#include "sc/token.hpp"      // For Token classes

//****************************************************************************
namespace sc {
//****************************************************************************
Token::Token(const SourceLocation &loc, TokenType type)
    : m_loc(loc), m_type(type) {}

std::ostream &Token::dump(std::ostream &out) const {
  out << get_location() << " TokenType: " << get_token_type();
  return out;
}

template <typename T>
TokenOf<T>::TokenOf(const SourceLocation &loc, TokenType type, const T &value)
    : Token(loc, type), m_value(value) {}

template <typename T> std::ostream &TokenOf<T>::dump(std::ostream &out) const {
  return Token::dump(out) << " Value: " << get_value();
}

std::ostream &operator<<(std::ostream &out, const Token &token) {
  return token.dump(out);
}

template class TokenOf<std::string>;
template class TokenOf<int>;
template class TokenOf<double>;
template class TokenOf<bool>;

std::string token_type_to_string(TokenType type) {
  switch (type) {
  case Token::EOF_TOKENTYPE:
    return "EOF";
  case Token::LEFT_PAREND:
    return "LEFT_PAREND";
  case Token::RIGHT_PAREND:
    return "RIGHT_PAREND";
  case Token::IDENTIFIER:
    return "IDENTIFIER";
  case Token::INT_LITERAL:
    return "INT_LITERAL";
  case Token::STRING_LITERAL:
    return "STRING_LITERAL";
  case Token::DOUBLE_LITERAL:
    return "DOUBLE_LITERAL";
  case Token::BOOL_LITERAL:
    return "BOOL_LITERAL";
  default:
    return "UNKNOWN(" + std::to_string(type) + ")";
  }
}

//****************************************************************************
} // namespace sc
//****************************************************************************
