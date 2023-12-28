//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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

#include "sc/sc.hpp"    // For forward decls
#include "sc/token.hpp" // For Token classes

//****************************************************************************
namespace sc {
//****************************************************************************
Token::Token(const SourceLocation &loc, TokenType type,
             const std::string &lexeme)
    : m_loc(loc), m_type(type), m_lexeme(lexeme) {}

Token::~Token() {}

bool Token::is_same_type_as(Token const &other) const {
  return m_type == other.m_type;
}

bool Token::is_same_lexeme_as(Token const &other) const {
  return m_lexeme == other.m_lexeme;
}

bool Token::operator==(Token const &other) const {
  return (m_loc == other.m_loc) && (is_same_type_as(other)) &&
         (is_same_lexeme_as(other));
}

std::ostream &Token::dump(std::ostream &out) {
  out << "Lexeme: \"" << m_lexeme << "\" ";
  m_loc.dump(out);
  out << " TokenType: " << m_type;
  return out;
}

template <typename T>
TokenOf<T>::TokenOf(const SourceLocation &loc, TokenType type,
                    const std::string &lexeme, const T &value)
    : Token(loc, type, lexeme), m_value(value) {}

template <typename T> TokenOf<T>::~TokenOf() {}

template <typename T>
bool TokenOf<T>::operator==(TokenOf<T> const &other) const {
  return this->Token::operator==(other) && (m_value == other.m_value);
}

template <typename T> std::ostream &TokenOf<T>::dump(std::ostream &out) {
  Token::dump(out);
  out << " Value: " << m_value;
  return out;
}

//****************************************************************************
} // namespace sc
//****************************************************************************
