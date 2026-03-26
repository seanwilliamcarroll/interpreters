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

#include "blip_tokens.hpp"
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
  while (peek().get_token_type() != BlipToken::EOF_TOKENTYPE) {
    expressions.push_back(parse_expression());
  }

  return std::make_unique<sc::ProgramNode>(initial_source,
                                           std::move(expressions));
}

template <typename InputToken, typename OutputAstNode>
std::unique_ptr<OutputAstNode> to_atom(Token *token) {
  auto literal_token = dynamic_cast<InputToken *>(token);
  return std::make_unique<OutputAstNode>(literal_token->get_location(),
                                         literal_token->get_value());
}

std::unique_ptr<sc::AstNode> Parser::parse_expression() {
  const auto &token = peek();

  switch (token.get_token_type()) {
  case BlipToken::BOOL_LITERAL:
    return to_atom<TokenBool, sc::BoolLiteral>(advance().get());
  case BlipToken::INT_LITERAL:
    return to_atom<TokenInt, sc::IntLiteral>(advance().get());
  case BlipToken::STRING_LITERAL:
    return to_atom<TokenString, sc::StringLiteral>(advance().get());
  case BlipToken::DOUBLE_LITERAL:
    return to_atom<TokenDouble, sc::DoubleLiteral>(advance().get());
  case BlipToken::IDENTIFIER:
    return to_atom<TokenIdentifier, sc::Identifier>(advance().get());
  case BlipToken::LEFT_PAREND:
    return parse_list();
  case BlipToken::RIGHT_PAREND:
    on_error(token.get_location(),
             "Unexpected Token of type: ", token.get_token_type());
  default:
    on_error(token.get_location(),
             "Unknown Token of type: ", token.get_token_type());
  }
}

std::unique_ptr<sc::AstNode> Parser::parse_list() {
  // When faced with a list, could be any number of things
  // - function call
  // - print/set/begin/if/while/defineVar/defineFn
  // Each having a particular style
  auto original_source_location = peek().get_location();
  expect(BlipToken::LEFT_PAREND, __FUNCTION__);

  std::vector<std::unique_ptr<sc::AstNode>> elements;

  while (peek().get_token_type() != BlipToken::RIGHT_PAREND) {
    switch (peek().get_token_type()) {
    case BlipToken::IDENTIFIER:
      elements.push_back(
          to_atom<TokenIdentifier, sc::Identifier>(advance().get()));
      break;
    case BlipToken::PRINT: {
      // Skip print token
      advance();
      auto node_to_print = parse_expression();
      expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
      return std::make_unique<PrintNode>(original_source_location,
                                         std::move(node_to_print));
    }
    default:
      on_error(peek().get_location(),
               "Unexpected Token in list of type: ", peek().get_token_type());
    }
  }

  return {};
}

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

std::unique_ptr<Token> Parser::expect(BlipTokenType token_type,
                                      const char *function_name) {
  const auto &current_token = peek();
  if (current_token.get_token_type() != token_type) {
    on_error(current_token.get_location(), function_name,
             " => Unexpected Token of type: ", current_token.get_token_type());
  }
  return advance();
}

//****************************************************************************
} // namespace blip
//****************************************************************************
