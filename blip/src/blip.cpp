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

#include <blip.hpp>     // For Blip
#include <sc/token.hpp> // For Token

//****************************************************************************
namespace blip {
//****************************************************************************

using namespace sc;

Blip::Blip(std::istream &in_stream, const std::string &file_name)
    : LanguageInterface(construct_lexer(in_stream, file_name)) {}

void Blip::rep() {
  std::unique_ptr<Token> next_token = nullptr;
  // For debug
  while (next_token == nullptr ||
         next_token->m_type != token_type::EOF_TOKENTYPE) {
    next_token = m_lexer->get_next_token();
    std::cout << "Got next token: ";
    std::cout.flush();
    next_token->dump(std::cout);
    std::cout << std::endl;
  }
}

void Blip::parse() {
  throw std::runtime_error("Blip::parse is unimplemented!");
}

void Blip::eval() { throw std::runtime_error("Blip::eval is unimplemented!"); }

std::unique_ptr<LexerInterface>
Blip::construct_lexer(std::istream &in_stream, const std::string &file_name) {
  return std::make_unique<Lexer>(in_stream, file_name);
}

//****************************************************************************
} // namespace blip
//****************************************************************************
