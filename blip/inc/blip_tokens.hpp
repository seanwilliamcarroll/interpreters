//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::TokenType enum and token type aliases
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string>

#include <lexer_interface.hpp>
#include <token.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

enum class TokenType : uint8_t {
  // Structural
  EOF_TOKEN = 0,
  LEFT_PAREND,
  RIGHT_PAREND,
  COLON,
  RIGHT_ARROW,
  // Literals
  IDENTIFIER,
  INT_LITERAL,
  STRING_LITERAL,
  DOUBLE_LITERAL,
  BOOL_LITERAL,
  // Keywords
  IF,
  WHILE,
  SET,
  BEGIN,
  PRINT,
  DEFINE,
};

// Type aliases
using Token = core::Token<TokenType>;
using TokenOf = core::TokenOf<TokenType, std::string>; // generic, rarely used
using TokenString = core::TokenOf<TokenType, std::string>;
using TokenInt = core::TokenOf<TokenType, int>;
using TokenDouble = core::TokenOf<TokenType, double>;
using TokenBool = core::TokenOf<TokenType, bool>;
using TokenIdentifier = core::TokenOf<TokenType, std::string>;
using LexerInterface = core::LexerInterface<TokenType>;

std::string token_type_to_string(TokenType type);

//****************************************************************************
} // namespace blip
//****************************************************************************
