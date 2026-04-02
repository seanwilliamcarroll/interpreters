//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of the bust driver class.
//*
//*
//****************************************************************************

#include "bust_tokens.hpp"
#include "lexer.hpp"
#include <bust.hpp>
#include <iostream>

//****************************************************************************

namespace bust {

Bust::Bust(std::istream &input, const char *filename)
    : m_input(input), m_filename(filename) {}

void Bust::rep() {
  // TODO: lex, parse, evaluate
  (void)m_input;
  (void)m_filename;

  auto lexer = make_lexer(m_input, m_filename);
  while (auto next_token = lexer->get_next_token()) {
    std::cout << *next_token << "\n";

    if (next_token->get_token_type() == TokenType::EOF_TOKEN) {
      break;
    }
  }
}

} // namespace bust

//****************************************************************************
