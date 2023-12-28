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

#include <memory>

#include <blip.hpp>                                    // For Blip
#include <sc/token.hpp>                                // For Token

//****************************************************************************
namespace blip {
//****************************************************************************

using namespace sc;
  
Blip::Blip(std::istream& in_stream, const std::string& file_name)
  : m_lexer(in_stream, file_name) {}

void Blip::rep() {
  std::unique_ptr<Token> next_token = nullptr;
  // For debug
  while (next_token == nullptr || next_token->m_type != token_type::EOF_TOKENTYPE) {
    next_token = m_lexer.getNextToken();
    std::cout << "Got next token: ";
    std::cout.flush();
    next_token->dump(std::cout);
    std::cout << std::endl;
  }
}


  
//****************************************************************************
} // namespace blip
//****************************************************************************
