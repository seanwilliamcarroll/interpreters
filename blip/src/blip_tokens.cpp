//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::token_type_to_string implementation
//*
//*
//****************************************************************************

#include <string>

#include <blip_tokens.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

std::string token_type_to_string(TokenType type) {
  switch (type) {
  case TokenType::EOF_TOKEN:
    return "EOF";
  case TokenType::LEFT_PAREND:
    return "LEFT_PAREND";
  case TokenType::RIGHT_PAREND:
    return "RIGHT_PAREND";
  case TokenType::COLON:
    return "COLON";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::INT_LITERAL:
    return "INT_LITERAL";
  case TokenType::STRING_LITERAL:
    return "STRING_LITERAL";
  case TokenType::DOUBLE_LITERAL:
    return "DOUBLE_LITERAL";
  case TokenType::BOOL_LITERAL:
    return "BOOL_LITERAL";
  case TokenType::IF:
    return "IF";
  case TokenType::WHILE:
    return "WHILE";
  case TokenType::SET:
    return "SET";
  case TokenType::BEGIN:
    return "BEGIN";
  case TokenType::PRINT:
    return "PRINT";
  case TokenType::DEFINE:
    return "DEFINE";
  }
  return "UNKNOWN(" + std::to_string(static_cast<unsigned int>(type)) + ")";
}

//****************************************************************************
} // namespace blip
//****************************************************************************
