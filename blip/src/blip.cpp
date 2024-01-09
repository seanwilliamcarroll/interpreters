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
      {{Token::EOF_TOKENTYPE, "Common::EOF_TOKENTYPE"},
       {Token::LEFT_PAREND, "Common::LEFT_PAREND"},
       {Token::RIGHT_PAREND, "Common::RIGHT_PAREND"},
       {Token::IDENTIFIER, "Common::IDENTIFIER"},
       {Token::INT_LITERAL, "Common::INT_LITERAL"},
       {Token::STRING_LITERAL, "Common::STRING_LITERAL"},
       {Token::DOUBLE_LITERAL, "Common::DOUBLE_LITERAL"},
       {Token::BOOL_LITERAL, "Common::BOOL_LITERAL"},
       {Blip::IF, "Blip::IF"},
       {Blip::WHILE, "Blip::WHILE"},
       {Blip::SET, "Blip::SET"},
       {Blip::BEGIN, "Blip::BEGIN"},
       {Blip::PRINT, "Blip::PRINT"},
       {Blip::DEFINE, "Blip::DEFINE"}};
  const auto &string_iter = token_type_strings.find(token.get_token_type());
  if (string_iter == token_type_strings.end()) {
    return out << "TokenType{"
               << std::to_string((unsigned int)(token.get_token_type())) << "}";
  }
  return out << string_iter->second;
}

Blip::Blip(std::istream &in_stream, const char *hint)
    : m_lexer(make_lexer(in_stream,
                         {{"if", IF},
                          {"while", WHILE},
                          {"set", SET},
                          {"begin", BEGIN},
                          {"print", PRINT},
                          {"define", DEFINE}},
                         hint)) {}

void Blip::rep() {
  std::unique_ptr<Token> next_token = nullptr;
  // For debug
  while (next_token == nullptr ||
         next_token->get_token_type() != Token::EOF_TOKENTYPE) {
    next_token = m_lexer->get_next_token();
    std::cout << "Got next token: " << *next_token << " (";
    dump_token_type(std::cout, *next_token) << ")" << std::endl;
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
