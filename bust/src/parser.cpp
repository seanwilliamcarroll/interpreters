//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : bust::Parser implementation
//*
//*
//****************************************************************************

#include <parser.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

// --- Token helpers ---------------------------------------------------------

const Token &Parser::peek() {
  if (m_current_token == nullptr) {
    m_current_token = m_lexer->get_next_token();
  }
  return *m_current_token;
}

std::unique_ptr<Token> Parser::advance() {
  if (m_current_token == nullptr) {
    m_current_token = m_lexer->get_next_token();
  }
  auto output = std::move(m_current_token);
  m_current_token = nullptr;
  return output;
}

std::unique_ptr<Token> Parser::expect(TokenType token_type,
                                      const char *function_name) {
  const auto &current_token = peek();
  if (current_token.get_token_type() != token_type) {
    on_error(current_token.get_location(), function_name,
             " => Expected: ", token_type_to_string(token_type),
             ", got: ", token_type_to_string(current_token.get_token_type()));
  }
  return advance();
}

//****************************************************************************
} // namespace bust
//****************************************************************************
