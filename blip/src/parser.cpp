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

#include "ast.hpp"
#include <memory>

#include <blip_tokens.hpp>
#include <numeric>
#include <parser.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

std::unique_ptr<ProgramNode> Parser::parse() {
  std::vector<std::unique_ptr<AstNode>> expressions;

  // Need to parse a list of expressions effectively and return them as a
  // program
  const auto initial_source = peek().get_location();
  while (peek().get_token_type() != TokenType::EOF_TOKEN) {
    expressions.push_back(parse_expression());
  }

  return std::make_unique<ProgramNode>(initial_source, std::move(expressions));
}

template <typename InputToken, typename OutputAstNode>
std::unique_ptr<OutputAstNode> to_atom(Token *token) {
  auto literal_token = dynamic_cast<InputToken *>(token);
  return std::make_unique<OutputAstNode>(literal_token->get_location(),
                                         literal_token->get_value());
}

std::unique_ptr<AstNode> Parser::parse_expression() {
  const auto &token = peek();

  switch (token.get_token_type()) {
  case TokenType::BOOL_LITERAL:
    return to_atom<TokenBool, BoolLiteral>(advance().get());
  case TokenType::INT_LITERAL:
    return to_atom<TokenInt, IntLiteral>(advance().get());
  case TokenType::STRING_LITERAL:
    return to_atom<TokenString, StringLiteral>(advance().get());
  case TokenType::DOUBLE_LITERAL:
    return to_atom<TokenDouble, DoubleLiteral>(advance().get());
  case TokenType::IDENTIFIER:
    return to_atom<TokenIdentifier, Identifier>(advance().get());
  case TokenType::LEFT_PAREND:
    return parse_list();
  case TokenType::RIGHT_PAREND:
    on_error(token.get_location(), "Unexpected token: ",
             token_type_to_string(token.get_token_type()));
  default:
    on_error(token.get_location(),
             "Unknown token: ", token_type_to_string(token.get_token_type()));
  }
}

std::unique_ptr<AstNode> Parser::parse_list() {
  // When faced with a list, could be any number of things
  // - function call
  // - print/set/begin/if/while/defineVar/defineFn
  // Each having a particular style
  auto original_source_location = peek().get_location();
  expect(TokenType::LEFT_PAREND, __FUNCTION__);

  std::vector<std::unique_ptr<AstNode>> elements;

  switch (peek().get_token_type()) {
  case TokenType::IF: {
    // Skip IF
    advance();
    auto condition = parse_expression();
    auto then_branch = parse_expression();
    std::unique_ptr<AstNode> else_branch = nullptr;
    if (peek().get_token_type() != TokenType::RIGHT_PAREND) {
      else_branch = parse_expression();
    }
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
    return std::make_unique<IfNode>(
        original_source_location, std::move(condition), std::move(then_branch),
        std::move(else_branch));
  }
  case TokenType::WHILE: {
    // Skip WHILE
    advance();
    auto condition = parse_expression();
    auto body = parse_expression();
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
    return std::make_unique<WhileNode>(original_source_location,
                                       std::move(condition), std::move(body));
  }
  case TokenType::SET: {
    // Skip SET
    advance();
    // Expect an IDENTIFIER
    if (peek().get_token_type() != TokenType::IDENTIFIER) {
      on_error(peek().get_location(), "Expected IDENTIFIER after SET, found: ",
               token_type_to_string(peek().get_token_type()));
    }
    auto identifier = to_atom<TokenIdentifier, Identifier>(advance().get());
    auto body = parse_expression();
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
    return std::make_unique<SetNode>(original_source_location,
                                     std::move(identifier), std::move(body));
  }
  case TokenType::BEGIN: {
    // Skip begin token
    auto begin_source_location = peek().get_location();
    advance();
    std::vector<std::unique_ptr<AstNode>> expressions;
    while (peek().get_token_type() != TokenType::RIGHT_PAREND) {
      expressions.push_back(parse_expression());
    }
    if (expressions.empty()) {
      on_error(begin_source_location, "BEGIN blocks cannot be empty!");
    }
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
    return std::make_unique<BeginNode>(begin_source_location,
                                       std::move(expressions));
  }
  case TokenType::PRINT: {
    // Skip print token
    advance();
    auto node_to_print = parse_expression();
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
    return std::make_unique<PrintNode>(original_source_location,
                                       std::move(node_to_print));
  }
  case TokenType::DEFINE: {
    // Skip define token
    advance();
    // Are we defining a function or a variable?
    std::unique_ptr<AstNode> defined_node = nullptr;
    if (peek().get_token_type() == TokenType::LEFT_PAREND) {
      defined_node = parse_function_definition(original_source_location);
    } else {
      // Could only be a variable then
      defined_node = parse_variable_definition(original_source_location);
    }
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
    return defined_node;
  }
  case TokenType::RIGHT_PAREND:
    on_error(peek().get_location(), "List should have at least one element!");
  default:
    // Must be a function call then
    auto callee = parse_expression();

    std::vector<std::unique_ptr<AstNode>> arguments;

    while (peek().get_token_type() != TokenType::RIGHT_PAREND) {
      arguments.push_back(parse_expression());
    }

    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
    return std::make_unique<CallNode>(original_source_location,
                                      std::move(callee), std::move(arguments));
  }
}

std::unique_ptr<BaseTypeNode> Parser::parse_type_node() {
  auto location = peek().get_location();
  switch (peek().get_token_type()) {
  case TokenType::IDENTIFIER:
    return to_atom<TokenIdentifier, TypeNode>(advance().get());
  case TokenType::LEFT_PAREND: {
    // Consume the parend
    advance();
    std::vector<std::unique_ptr<BaseTypeNode>> arguments;
    while (peek().get_token_type() != TokenType::RIGHT_ARROW) {
      arguments.push_back(parse_type_node());
    }

    expect(TokenType::RIGHT_ARROW, __FUNCTION__);

    auto return_type = parse_type_node();
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);

    return std::make_unique<FunctionTypeNode>(location, std::move(arguments),
                                              std::move(return_type));
  }
  default:
    on_error(peek().get_location(),
             "Expect either IDENTIFIER or LEFT_PAREND as type signature, not:",
             token_type_to_string(peek().get_token_type()));
  }
}

std::unique_ptr<DefineFnNode> Parser::parse_function_definition(
    const SourceLocation &original_source_location) {
  // Function
  advance();
  auto [name, arguments] = parse_function_declaration();
  expect(TokenType::RIGHT_PAREND, __FUNCTION__);

  // Get type of this function
  expect(TokenType::COLON, __FUNCTION__);
  auto return_type_node = parse_type_node();

  auto body = parse_expression();
  return std::make_unique<DefineFnNode>(
      original_source_location, std::move(name), std::move(arguments),
      std::move(body), std::move(return_type_node));
}

std::unique_ptr<DefineVarNode> Parser::parse_variable_definition(
    const SourceLocation &original_source_location) {
  if (peek().get_token_type() != TokenType::IDENTIFIER) {
    on_error(peek().get_location(),
             "Expected IDENTIFIER after DEFINE token, not: ",
             token_type_to_string(peek().get_token_type()));
  }
  auto name = to_atom<TokenIdentifier, Identifier>(advance().get());

  std::unique_ptr<BaseTypeNode> type_node{};

  if (peek().get_token_type() == TokenType::COLON) {
    advance();
    type_node = parse_type_node();
  }

  auto body = parse_expression();
  return std::make_unique<DefineVarNode>(original_source_location,
                                         std::move(name), std::move(body),
                                         std::move(type_node));
}

std::pair<std::unique_ptr<Identifier>, std::vector<std::unique_ptr<Identifier>>>
Parser::parse_function_declaration() {
  std::vector<std::unique_ptr<Identifier>> identifiers;

  if (peek().get_token_type() != TokenType::IDENTIFIER) {
    on_error(peek().get_location(),
             "Must have at least one identifier in function definition!");
  }

  auto name = to_atom<TokenIdentifier, Identifier>(advance().get());

  while (peek().get_token_type() == TokenType::LEFT_PAREND) {
    advance();

    if (peek().get_token_type() != TokenType::IDENTIFIER) {
      on_error(peek().get_location(), "Expect identifier as parameter, not:",
               token_type_to_string(peek().get_token_type()));
    }
    auto parameter_token = advance();
    expect(TokenType::COLON, __FUNCTION__);
    auto type_node = parse_type_node();
    auto literal_token = dynamic_cast<TokenIdentifier *>(parameter_token.get());
    identifiers.push_back(std::make_unique<Identifier>(
        literal_token->get_location(), literal_token->get_value(),
        std::move(type_node)));
    expect(TokenType::RIGHT_PAREND, __FUNCTION__);
  }

  return {std::move(name), std::move(identifiers)};
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

std::unique_ptr<Token> Parser::expect(TokenType token_type,
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
