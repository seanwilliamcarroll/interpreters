//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Blip source file
//*
//*
//****************************************************************************

#include "blip_tokens.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include <blip.hpp>               // For Blip
#include <sc/exceptions.hpp>      // For UnknownTokenTypeException
#include <sc/lexer_interface.hpp> // For LexerInterface
#include <sc/sc.hpp>              // For TokenType
#include <sc/token.hpp>           // For Token, CommonTokenEnum

//****************************************************************************
namespace blip {
//****************************************************************************

using namespace sc;

std::ostream &dump_token_type(std::ostream &out, const Token &token) {
  static const std::unordered_map<TokenType, const char *> token_type_strings =
      {{BlipToken::EOF_TOKENTYPE, "Common::EOF_TOKENTYPE"},
       {BlipToken::LEFT_PAREND, "Common::LEFT_PAREND"},
       {BlipToken::RIGHT_PAREND, "Common::RIGHT_PAREND"},
       {BlipToken::IDENTIFIER, "Common::IDENTIFIER"},
       {BlipToken::INT_LITERAL, "Common::INT_LITERAL"},
       {BlipToken::STRING_LITERAL, "Common::STRING_LITERAL"},
       {BlipToken::DOUBLE_LITERAL, "Common::DOUBLE_LITERAL"},
       {BlipToken::BOOL_LITERAL, "Common::BOOL_LITERAL"},
       {BlipToken::IF, "Blip::IF"},
       {BlipToken::WHILE, "Blip::WHILE"},
       {BlipToken::SET, "Blip::SET"},
       {BlipToken::BEGIN, "Blip::BEGIN"},
       {BlipToken::PRINT, "Blip::PRINT"},
       {BlipToken::DEFINE, "Blip::DEFINE"}};
  const auto &string_iter = token_type_strings.find(token.get_token_type());
  if (string_iter == token_type_strings.end()) {
    return out << "TokenType{"
               << std::to_string((unsigned int)(token.get_token_type())) << "}";
  }
  return out << string_iter->second;
}

Blip::Blip(std::istream &in_stream, const char *hint)
    : m_lexer(make_lexer(in_stream,
                         {{"if", BlipToken::IF},
                          {"while", BlipToken::WHILE},
                          {"set", BlipToken::SET},
                          {"begin", BlipToken::BEGIN},
                          {"print", BlipToken::PRINT},
                          {"define", BlipToken::DEFINE}},
                         hint)) {}

void Blip::rep() {
  std::unique_ptr<Token> next_token = nullptr;
  // For debug
  while (next_token == nullptr ||
         next_token->get_token_type() != BlipToken::EOF_TOKENTYPE) {
    next_token = m_lexer->get_next_token();
    std::cout << "Got next token: " << *next_token << " (";
    dump_token_type(std::cout, *next_token) << ")" << std::endl;
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
