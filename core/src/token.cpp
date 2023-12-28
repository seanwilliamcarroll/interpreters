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

#include "sc/sc.hpp"                                   // For forward decls
#include "sc/token.hpp"                                // For Token classes

//****************************************************************************
namespace sc {
//****************************************************************************

std::ostream& Token::dump(std::ostream& out) {
  out << "Lexeme: \"" << m_lexeme << "\" ";
  m_loc.dump(out);
  out << " TokenType: " << m_type;
  return out;
}

template <typename T>
std::ostream& TokenOf<T>::dump(std::ostream& out) {
  Token::dump(out);
  out << " Value: " << m_value;
  return out;
}
  
  
//****************************************************************************
} // namespace sc
//****************************************************************************
