//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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

#include <blip.hpp>               // For Blip
#include <sc/lexer_interface.hpp> // For LexerInterface
#include <sc/sc.hpp>              // For TokenType
#include <sc/token.hpp>           // For Token, CommonTokenEnum

//****************************************************************************
namespace blip {
//****************************************************************************

using namespace sc;

enum BlipTokenType : TokenType {
  INVALID_START = CommonTokenEnum::INVALID_END,

  IF,
  WHILE,
  SET,
  BEGIN,
  PRINT,

  INVALID_END
};

struct BlipTokenEnum {
  using enum BlipTokenType;
};

Blip::Blip(std::istream &in_stream, const char *hint)
    : m_lexer(make_lexer(in_stream,
                         {{"if", BlipTokenEnum::IF},
                          {"while", BlipTokenEnum::WHILE},
                          {"set", BlipTokenEnum::SET},
                          {"begin", BlipTokenEnum::BEGIN},
                          {"print", BlipTokenEnum::PRINT}},
                         hint)) {}

void Blip::rep() {
  std::unique_ptr<Token> next_token = nullptr;
  // For debug
  while (next_token == nullptr ||
         next_token->m_type != CommonTokenEnum::EOF_TOKENTYPE) {
    next_token = m_lexer->get_next_token();
    std::cout << "Got next token: ";
    std::cout.flush();
    next_token->dump(std::cout);
    std::cout << std::endl;
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
