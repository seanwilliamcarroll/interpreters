//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of token_type_to_string for bust tokens.
//*
//*
//****************************************************************************

#include <bust_tokens.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

std::string token_type_to_string(TokenType type) {
  switch (type) {
  case TokenType::EOF_TOKEN:
    return "EOF";
  case TokenType::LPAREN:
    return "(";
  case TokenType::RPAREN:
    return ")";
  case TokenType::LBRACE:
    return "{";
  case TokenType::RBRACE:
    return "}";
  case TokenType::ARROW:
    return "->";
  case TokenType::COLON:
    return ":";
  case TokenType::SEMICOLON:
    return ";";
  case TokenType::COMMA:
    return ",";
  case TokenType::EQUALS:
    return "=";
  case TokenType::PIPE:
    return "|";
  case TokenType::AND:
    return "&";
  case TokenType::PLUS:
    return "+";
  case TokenType::MINUS:
    return "-";
  case TokenType::STAR:
    return "*";
  case TokenType::SLASH:
    return "/";
  case TokenType::PERCENT:
    return "%";
  case TokenType::EQ_EQ:
    return "==";
  case TokenType::BANG_EQ:
    return "!=";
  case TokenType::LESS:
    return "<";
  case TokenType::GREATER:
    return ">";
  case TokenType::LESS_EQ:
    return "<=";
  case TokenType::GREATER_EQ:
    return ">=";
  case TokenType::AND_AND:
    return "&&";
  case TokenType::OR_OR:
    return "||";
  case TokenType::BANG:
    return "!";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::INT_LITERAL:
    return "INT_LITERAL";
  case TokenType::FN:
    return "fn";
  case TokenType::LET:
    return "let";
  case TokenType::RETURN:
    return "return";
  case TokenType::IF:
    return "if";
  case TokenType::ELSE:
    return "else";
  case TokenType::WHILE:
    return "while";
  case TokenType::FOR:
    return "for";
  case TokenType::TRUE:
    return "true";
  case TokenType::FALSE:
    return "false";
  case TokenType::I64:
    return "i64";
  case TokenType::BOOL:
    return "bool";
  case TokenType::UNIT:
    return "()";
  }
}

std::ostream &operator<<(std::ostream &out, const TokenType &token_type) {
  return out << token_type_to_string(token_type);
}

//****************************************************************************
} // namespace bust
//****************************************************************************
