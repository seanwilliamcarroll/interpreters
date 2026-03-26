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

  auto to_literal =
      []<typename InputToken, typename OutputAstNode>(sc::Token *token) {
        auto literal_token = dynamic_cast<InputToken *>(token);
        return std::make_unique<OutputAstNode>(literal_token->get_location(),
                                               literal_token->get_value());
      };

  switch (token->get_token_type()) {
  case sc::Token::BOOL_LITERAL:
    return to_literal.operator()<sc::TokenBool, sc::BoolLiteral>(token.get());
  case sc::Token::INT_LITERAL:
    return to_literal.operator()<sc::TokenInt, sc::IntLiteral>(token.get());
  case sc::Token::STRING_LITERAL:
    return to_literal.operator()<sc::TokenString, sc::StringLiteral>(
        token.get());
  case sc::Token::DOUBLE_LITERAL:
    return to_literal.operator()<sc::TokenDouble, sc::DoubleLiteral>(
        token.get());
  default:
    on_error(token->get_location(),
             "Unexpected Token of type: ", token->get_token_type());
  }
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
