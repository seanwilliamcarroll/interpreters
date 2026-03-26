//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Parser implementation
//*
//*
//****************************************************************************

#include "sc/sc.hpp"
#include <memory>
#include <parser.hpp>

#include <sc/ast.hpp>
#include <sc/token.hpp>

#include <ast.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

std::unique_ptr<sc::AstNode> Parser::parse() {
  std::vector<std::unique_ptr<sc::AstNode>> expressions;

  // Need to parse a list of expressions effectively and return them as a
  // program
  const auto initial_source = peek().get_location();
  while (peek().get_token_type() != sc::Token::EOF_TOKENTYPE) {
    expressions.push_back(parse_expression());
  }

  return std::make_unique<sc::ProgramNode>(initial_source,
                                           std::move(expressions));
}

std::unique_ptr<sc::AstNode> Parser::parse_expression() {
  auto token = advance();
  switch (token->get_token_type()) {
  case sc::Token::BOOL_LITERAL: {
    auto bool_token = dynamic_cast<sc::TokenBool *>(token.get());
    return std::make_unique<sc::BoolLiteral>(bool_token->get_location(),
                                             bool_token->get_value());
  }
  case sc::Token::INT_LITERAL: {
    auto int_token = dynamic_cast<sc::TokenInt *>(token.get());
    return std::make_unique<sc::IntLiteral>(int_token->get_location(),
                                            int_token->get_value());
  }
  case sc::Token::STRING_LITERAL: {
    auto string_token = dynamic_cast<sc::TokenString *>(token.get());
    return std::make_unique<sc::StringLiteral>(string_token->get_location(),
                                               string_token->get_value());
  }
  case sc::Token::DOUBLE_LITERAL: {
    auto double_token = dynamic_cast<sc::TokenDouble *>(token.get());
    return std::make_unique<sc::DoubleLiteral>(double_token->get_location(),
                                               double_token->get_value());
  }
  default:
    on_error(token->get_location(),
             "Unexpected Token of type: ", peek().get_token_type());
  }

  return {};
}

const sc::Token &Parser::peek() {
  if (m_current_token == nullptr) {
    m_current_token = m_lexer->get_next_token();
  }
  return *m_current_token;
}

std::unique_ptr<sc::Token> Parser::advance() {
  if (m_current_token == nullptr) {
    m_current_token = m_lexer->get_next_token();
  }
  auto output = std::move(m_current_token);
  m_current_token = nullptr;
  return output;
}

std::unique_ptr<sc::Token> Parser::expect(sc::TokenType token_type) {
  const auto &current_token = peek();
  if (current_token.get_token_type() != token_type) {
    on_error(current_token.get_location(),
             "Unexpected Token of type: ", current_token.get_token_type());
  }
  return advance();
}

//****************************************************************************
} // namespace blip
//****************************************************************************
