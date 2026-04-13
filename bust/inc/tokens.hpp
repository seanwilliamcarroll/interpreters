//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : bust::TokenType enum and token type aliases
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>

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
  AS,

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
  CHAR_LITERAL,

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
  I8,
  I32,
  I64,
  BOOL,
  CHAR,
  UNIT,
};

std::ostream &operator<<(std::ostream &, const TokenType &);

// Type aliases
using Token = core::Token<TokenType>;
using TokenIdentifier = core::TokenOf<TokenType, std::string>;
using TokenNumber = core::TokenOf<TokenType, std::string>;
using TokenChar = core::TokenOf<TokenType, std::string>;
using LexerInterface = core::LexerInterface<TokenType>;

std::string token_type_to_string(TokenType type);

inline constexpr std::array<std::pair<std::string_view, TokenType>, 15>
    keywords{{
        {"fn", TokenType::FN},
        {"let", TokenType::LET},
        {"return", TokenType::RETURN},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"true", TokenType::TRUE},
        {"false", TokenType::FALSE},
        {"as", TokenType::AS},
        {"i8", TokenType::I8},
        {"i32", TokenType::I32},
        {"i64", TokenType::I64},
        {"bool", TokenType::BOOL},
        {"char", TokenType::CHAR},
    }};

//****************************************************************************
} // namespace bust
//****************************************************************************
