//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : bust::TokenType enum and token type aliases
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cstdint>
#include <iosfwd>
#include <string>

#include <lexer_interface.hpp>
#include <token.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

enum class TokenType : uint8_t {
  // Structural
  EOF_TOKEN = 0,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  ARROW,
  COLON,
  SEMICOLON,
  COMMA,
  EQUALS,
  PIPE,
  AND,

  // Arithmetic operators
  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,

  // Comparison operators
  EQ_EQ,
  BANG_EQ,
  LESS,
  GREATER,
  LESS_EQ,
  GREATER_EQ,

  // Logical operators
  AND_AND,
  OR_OR,
  BANG,

  // Literals
  IDENTIFIER,
  INT_LITERAL,

  // Keywords
  FN,
  LET,
  RETURN,
  IF,
  ELSE,
  WHILE,
  FOR,
  TRUE,
  FALSE,

  // Type keywords
  I64,
  BOOL,
  UNIT,
};

std::ostream &operator<<(std::ostream &, const TokenType &);

// Type aliases
using Token = core::Token<TokenType>;
using TokenIdentifier = core::TokenOf<TokenType, std::string>;
using TokenNumber = core::TokenOf<TokenType, std::string>;
using LexerInterface = core::LexerInterface<TokenType>;

std::string token_type_to_string(TokenType type);

//****************************************************************************
} // namespace bust
//****************************************************************************
