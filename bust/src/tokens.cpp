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

#include <ostream>
#include <tokens.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

std::string token_type_to_string(TokenType type) {
  switch (type) {
  case TokenType::EOF_TOKEN:
    return "EOF";
  // Single-char structural/operators
  case TokenType::LPAREN:
    return "LPAREN '('";
  case TokenType::RPAREN:
    return "RPAREN ')'";
  case TokenType::LBRACE:
    return "LBRACE '{'";
  case TokenType::RBRACE:
    return "RBRACE '}'";
  case TokenType::COLON:
    return "COLON ':'";
  case TokenType::SEMICOLON:
    return "SEMICOLON ';'";
  case TokenType::COMMA:
    return "COMMA ','";
  case TokenType::EQUALS:
    return "EQUALS '='";
  case TokenType::PIPE:
    return "PIPE '|'";
  case TokenType::AND:
    return "AND '&'";
  case TokenType::PLUS:
    return "PLUS '+'";
  case TokenType::MINUS:
    return "MINUS '-'";
  case TokenType::STAR:
    return "STAR '*'";
  case TokenType::SLASH:
    return "SLASH '/'";
  case TokenType::PERCENT:
    return "PERCENT '%'";
  case TokenType::LESS:
    return "LESS '<'";
  case TokenType::GREATER:
    return "GREATER '>'";
  case TokenType::BANG:
    return "BANG '!'";
  // Multi-char structural/operators
  case TokenType::ARROW:
    return "ARROW \"->\"";
  case TokenType::EQ_EQ:
    return "EQ_EQ \"==\"";
  case TokenType::BANG_EQ:
    return "BANG_EQ \"!=\"";
  case TokenType::LESS_EQ:
    return "LESS_EQ \"<=\"";
  case TokenType::GREATER_EQ:
    return "GREATER_EQ \">=\"";
  case TokenType::AND_AND:
    return "AND_AND \"&&\"";
  case TokenType::OR_OR:
    return "OR_OR \"||\"";
  case TokenType::UNIT:
    return "UNIT \"()\"";
  // Literals (lexeme carried in token value)
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::INT_LITERAL:
    return "INT_LITERAL";
  case TokenType::CHAR_LITERAL:
    return "CHAR_LITERAL";
  // Keywords
  case TokenType::FN:
    return "FN \"fn\"";
  case TokenType::LET:
    return "LET \"let\"";
  case TokenType::RETURN:
    return "RETURN \"return\"";
  case TokenType::IF:
    return "IF \"if\"";
  case TokenType::ELSE:
    return "ELSE \"else\"";
  case TokenType::WHILE:
    return "WHILE \"while\"";
  case TokenType::FOR:
    return "FOR \"for\"";
  case TokenType::AS:
    return "AS \"as\"";
  case TokenType::TRUE:
    return "TRUE \"true\"";
  case TokenType::FALSE:
    return "FALSE \"false\"";
  case TokenType::I8:
    return "I8 \"i8\"";
  case TokenType::I32:
    return "I32 \"i32\"";
  case TokenType::I64:
    return "I64 \"i64\"";
  case TokenType::BOOL:
    return "BOOL \"bool\"";
  case TokenType::CHAR:
    return "CHAR \"char\"";
  }
}

std::ostream &operator<<(std::ostream &out, const TokenType &token_type) {
  return out << token_type_to_string(token_type);
}

//****************************************************************************
} // namespace bust
//****************************************************************************
