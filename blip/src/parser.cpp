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
    on_error(token.get_location(), "Unexpected token: ",
             token_type_to_string(token.get_token_type()));
  default:
    on_error(token.get_location(),
             "Unknown token: ", token_type_to_string(token.get_token_type()));
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
    case BlipToken::IF: {
      auto begin_source_location = peek().get_location();
      // Skip IF
      advance();
      auto condition = parse_expression();
      auto then_branch = parse_expression();
      std::unique_ptr<sc::AstNode> else_branch = nullptr;
      if (peek().get_token_type() != BlipToken::RIGHT_PAREND) {
        else_branch = parse_expression();
      }
      expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
      return std::make_unique<IfNode>(
          original_source_location, std::move(condition),
          std::move(then_branch), std::move(else_branch));
    }
    case BlipToken::WHILE: {
      auto begin_source_location = peek().get_location();
      // Skip WHILE
      advance();
      auto condition = parse_expression();
      auto body = parse_expression();
      expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
      return std::make_unique<WhileNode>(original_source_location,
                                         std::move(condition), std::move(body));
    }
    case BlipToken::SET: {
      auto begin_source_location = peek().get_location();
      // Skip SET
      advance();
      // Expect an IDENTIFIER
      if (peek().get_token_type() != BlipToken::IDENTIFIER) {
        on_error(peek().get_location(),
                 "Expected IDENTIFIER after SET, found: ",
                 token_type_to_string(peek().get_token_type()));
      }
      auto identifier =
          to_atom<TokenIdentifier, sc::Identifier>(advance().get());

      auto body = parse_expression();
      expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
      return std::make_unique<SetNode>(original_source_location,
                                       std::move(identifier), std::move(body));
    }
    case BlipToken::BEGIN: {
      // Skip begin token
      auto begin_source_location = peek().get_location();
      advance();
      std::vector<std::unique_ptr<sc::AstNode>> expressions;
      while (peek().get_token_type() != BlipToken::RIGHT_PAREND) {
        expressions.push_back(parse_expression());
      }
      if (expressions.empty()) {
        on_error(begin_source_location, "BEGIN blocks cannot be empty!");
      }
      expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
      return std::make_unique<BeginNode>(begin_source_location,
                                         std::move(expressions));
    }
    case BlipToken::PRINT: {
      // Skip print token
      advance();
      auto node_to_print = parse_expression();
      expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
      return std::make_unique<PrintNode>(original_source_location,
                                         std::move(node_to_print));
    }
    case BlipToken::DEFINE: {
      // Skip define token
      advance();
      // Are we defining a function or a variable?
      if (peek().get_token_type() == BlipToken::LEFT_PAREND) {
        // Function
        // Need to parse identifier list, requires at least one, the function
        // name
        advance();
        auto arguments = parse_identifier_list();
        if (arguments.empty()) {
          on_error(peek().get_location(),
                   "Must have at least one identifier in function definition!");
        }
        auto name = std::move(arguments.front());
        arguments.erase(arguments.begin());
        expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
        auto body = parse_expression();
        expect(BlipToken::RIGHT_PAREND, __FUNCTION__);
        return std::make_unique<DefineFnNode>(
            original_source_location, std::move(name), std::move(arguments),
            std::move(body));
      } else {
        // Variable
      }
    }
    default:
      on_error(peek().get_location(), "Unexpected token in list: ",
               token_type_to_string(peek().get_token_type()));
    }
  }

  return {};
}

std::vector<std::unique_ptr<sc::Identifier>> Parser::parse_identifier_list() {

  std::vector<std::unique_ptr<sc::Identifier>> identifiers;

  while (peek().get_token_type() == BlipToken::IDENTIFIER) {
    identifiers.push_back(
        to_atom<TokenIdentifier, sc::Identifier>(advance().get()));
  }

  return identifiers;
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
             " => Unexpected token: ",
             token_type_to_string(current_token.get_token_type()));
  }
  return advance();
}

//****************************************************************************
} // namespace blip
//****************************************************************************
