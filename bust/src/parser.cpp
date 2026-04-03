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

#include "ast.hpp"
#include "bust_tokens.hpp"
#include "source_location.hpp"
#include <memory>
#include <optional>
#include <parser.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

//****************************************************************************
namespace bust {
//****************************************************************************

// --- Public ----------------------------------------------------------------

Program Parser::parse() { return parse_program(); }

// --- Top-level -------------------------------------------------------------

Program Parser::parse_program() {

  auto starting_location = peek().get_location();

  std::vector<TopItem> top_items;

  while (peek().get_token_type() != TokenType::EOF_TOKEN) {
    top_items.push_back(parse_top_item());
  }

  return {{.m_location = starting_location}, std::move(top_items)};
}

TopItem Parser::parse_top_item() {
  if (peek().get_token_type() == TokenType::FN) {
    return std::make_unique<FunctionDef>(parse_func_def());
  }
  if (peek().get_token_type() == TokenType::LET) {
    return std::make_unique<LetBinding>(parse_let_binding());
  }

  on_error(peek().get_location(),
           "parse_top_item error: Expected LET or FN, found TokenType: ",
           peek().get_token_type());
}

std::pair<core::SourceLocation, std::string>
Parser::parse_location_name_from_identifier(const char *error_message) {
  if (peek().get_token_type() != TokenType::IDENTIFIER) {
    on_error(peek().get_location(), error_message,
             ", expected IDENTIFIER, not TokenType: ", peek().get_token_type());
  }

  const auto &token = advance();
  const auto *identifier_token_ptr =
      dynamic_cast<const TokenIdentifier *>(token.get());
  if (identifier_token_ptr == nullptr) {
    on_error(peek().get_location(),
             "Unable to cast to TokenIdentifier: ", token->get_token_type());
  }
  return {
      identifier_token_ptr->get_location(),
      identifier_token_ptr->get_value(),
  };
}

TypeIdentifier Parser::parse_type_identifier() {
  switch (peek().get_token_type()) {
  case TokenType::UNIT:
    return PrimitiveTypeIdentifier{{advance()->get_location()},
                                   PrimitiveType::UNIT};
  case TokenType::BOOL:
    return PrimitiveTypeIdentifier{{advance()->get_location()},
                                   PrimitiveType::BOOL};
  case TokenType::I64:
    return PrimitiveTypeIdentifier{{advance()->get_location()},
                                   PrimitiveType::INT64};
  default:
    const auto [location, identifier_name] =
        parse_location_name_from_identifier("Malformed type annotation");

    return DefinedType{
        {location},
        identifier_name,
    };
  }
}

TypeIdentifier Parser::parse_type_annotation() {
  expect(TokenType::COLON, __FUNCTION__);
  return parse_type_identifier();
}

TypeIdentifier Parser::parse_function_type_annotation() {
  expect(TokenType::ARROW, __FUNCTION__);
  return parse_type_identifier();
}

Identifier Parser::parse_non_annotated_identifier() {
  const auto [location, identifier_name] =
      parse_location_name_from_identifier("Malformed parameter");

  if (peek().get_token_type() == TokenType::COLON) {
    on_error(peek().get_location(),
             "Expected Identifier without Type Annotation here!");
  }

  return {{location}, identifier_name, std::nullopt};
}

Identifier Parser::parse_possibly_annotated_identifier() {
  const auto [location, identifier_name] =
      parse_location_name_from_identifier("Malformed parameter");

  auto type = (peek().get_token_type() == TokenType::COLON)
                  ? std::optional(parse_type_annotation())
                  : std::nullopt;

  return {{location}, identifier_name, type};
}

Identifier Parser::parse_annotated_identifier() {
  const auto [location, identifier_name] =
      parse_location_name_from_identifier("Malformed parameter");

  auto type = parse_type_annotation();
  ;

  return {{location}, identifier_name, type};
}

FunctionDef Parser::parse_func_def() {
  auto original_location = peek().get_location();
  expect(TokenType::FN, __FUNCTION__);

  auto function_id = parse_non_annotated_identifier();

  std::vector<Identifier> parameters{};
  if (peek().get_token_type() == TokenType::UNIT) {
    advance();
  } else {
    expect(TokenType::LPAREN, __FUNCTION__);

    parameters = parse_param_list();

    expect(TokenType::RPAREN, __FUNCTION__);
  }

  auto return_type = (peek().get_token_type() == TokenType::ARROW)
                         ? parse_function_type_annotation()
                         : PrimitiveTypeIdentifier{
                               {function_id.m_location},
                               PrimitiveType::UNIT,
                           };

  auto body = parse_block();

  return {{original_location},
          function_id,
          parameters,
          return_type,
          std::move(body)};
}

LetBinding Parser::parse_let_binding() {
  auto original_location = peek().get_location();
  expect(TokenType::LET, __FUNCTION__);

  auto identifier = parse_possibly_annotated_identifier();

  expect(TokenType::EQUALS, __FUNCTION__);

  auto body = parse_expression();

  expect(TokenType::SEMICOLON, __FUNCTION__);

  return {{original_location}, identifier, std::move(body)};
}

std::vector<Identifier> Parser::parse_param_list() {
  std::vector<Identifier> parameters;
  while (peek().get_token_type() != TokenType::RPAREN) {
    parameters.push_back(parse_annotated_identifier());
    if (peek().get_token_type() != TokenType::RPAREN) {
      expect(TokenType::COMMA, __FUNCTION__);
    }
  }
  return parameters;
}

std::vector<Identifier> Parser::parse_lambda_param_list() {
  std::vector<Identifier> parameters;
  while (peek().get_token_type() != TokenType::RPAREN) {
    // Could force all or nothing, either they all have an annotation or none of
    // them
    // Flexible for now
    parameters.push_back(parse_possibly_annotated_identifier());
    if (peek().get_token_type() != TokenType::RPAREN) {
      expect(TokenType::COMMA, __FUNCTION__);
    }
  }
  return parameters;
}

// --- Blocks ----------------------------------------------------------------

Block Parser::parse_block() {
  auto original_location = peek().get_location();
  expect(TokenType::LBRACE, __FUNCTION__);

  std::vector<Statement> statements;
  while (peek().get_token_type() != TokenType::RBRACE) {
    if (peek().get_token_type() == TokenType::LET) {
      auto let_binding = std::make_unique<LetBinding>(parse_let_binding());
      statements.emplace_back(std::move(let_binding));
      continue;
    }
    auto next_expression = parse_expression();
    if (peek().get_token_type() == TokenType::SEMICOLON) {
      advance();
      statements.emplace_back(std::move(next_expression));
      continue;
    }
    auto final_expression = std::optional(std::move(next_expression));

    expect(TokenType::RBRACE, __FUNCTION__);

    return {{original_location},
            std::move(statements),
            std::move(final_expression)};
  }

  expect(TokenType::RBRACE, __FUNCTION__);

  return {{original_location}, std::move(statements), std::nullopt};
}

// --- Expression precedence chain -------------------------------------------

Expression Parser::parse_expression() { return parse_logic_or(); }

Expression Parser::parse_binary_expression(
    const std::unordered_map<TokenType, BinaryOperator> &possible_mappings,
    auto next_clause_to_parse) {
  auto original_location = peek().get_location();

  auto expression = next_clause_to_parse();

  auto iter = possible_mappings.find(peek().get_token_type());

  while (iter != possible_mappings.end()) {
    auto op = iter->second;
    advance();
    expression =
        std::make_unique<BinaryExpr>(BinaryExpr{{original_location},
                                                op,
                                                std::move(expression),
                                                next_clause_to_parse()});
    iter = possible_mappings.find(peek().get_token_type());
  }

  return expression;
}

Expression Parser::parse_logic_or() {
  return parse_binary_expression(
      {{TokenType::OR_OR, BinaryOperator::LOGICAL_OR}},
      [this] { return parse_logic_and(); });
}

Expression Parser::parse_logic_and() {
  return parse_binary_expression(
      {{TokenType::AND_AND, BinaryOperator::LOGICAL_AND}},
      [this] { return parse_equality(); });
}

Expression Parser::parse_equality() {
  return parse_binary_expression(
      {
          {TokenType::EQ_EQ, BinaryOperator::EQ},
          {TokenType::BANG_EQ, BinaryOperator::NOT_EQ},
      },
      [this] { return parse_comparison(); });
}

Expression Parser::parse_comparison() {
  return parse_binary_expression(
      {
          {TokenType::LESS, BinaryOperator::LT},
          {TokenType::GREATER, BinaryOperator::GT},
          {TokenType::LESS_EQ, BinaryOperator::LT_EQ},
          {TokenType::GREATER_EQ, BinaryOperator::GT_EQ},
      },
      [this] { return parse_add_sub(); });
}

Expression Parser::parse_add_sub() {
  return parse_binary_expression(
      {
          {TokenType::PLUS, BinaryOperator::PLUS},
          {TokenType::MINUS, BinaryOperator::MINUS},
      },
      [this] { return parse_mult_div_mod(); });
}

Expression Parser::parse_mult_div_mod() {
  return parse_binary_expression(
      {
          {TokenType::STAR, BinaryOperator::MULTIPLIES},
          {TokenType::SLASH, BinaryOperator::DIVIDES},
          {TokenType::PERCENT, BinaryOperator::MODULUS},
      },
      [this] { return parse_unary_pre(); });
}

Expression Parser::parse_unary_pre() {
  const static std::unordered_map<TokenType, UnaryOperator> possible_mappings =
      {
          {TokenType::MINUS, UnaryOperator::MINUS},
          {TokenType::BANG, UnaryOperator::NOT},
      };

  auto iter = possible_mappings.find(peek().get_token_type());
  if (iter != possible_mappings.end()) {
    auto original_location = peek().get_location();
    auto op = iter->second;
    advance();
    return std::make_unique<UnaryExpr>(
        UnaryExpr{{original_location}, op, parse_postfix()});
  }

  return parse_postfix();
}

Expression Parser::parse_postfix() {
  auto original_location = peek().get_location();

  auto expression = parse_primary();

  while (peek().get_token_type() == TokenType::LPAREN ||
         peek().get_token_type() == TokenType::UNIT) {
    if (peek().get_token_type() == TokenType::UNIT) {
      advance();
      expression = std::make_unique<CallExpr>(
          CallExpr{{original_location}, std::move(expression), {}});
      continue;
    }

    expect(TokenType::LPAREN, __FUNCTION__);

    std::vector<Expression> arguments;
    while (peek().get_token_type() != TokenType::RPAREN) {
      arguments.push_back(parse_expression());

      if (peek().get_token_type() != TokenType::RPAREN) {
        expect(TokenType::COMMA, __FUNCTION__);
      }
    }

    expect(TokenType::RPAREN, __FUNCTION__);
    expression = std::make_unique<CallExpr>(CallExpr{
        {original_location}, std::move(expression), std::move(arguments)});
  }

  return expression;
}

Expression Parser::parse_primary() {
  switch (peek().get_token_type()) {
  case TokenType::INT_LITERAL:
  case TokenType::TRUE:
  case TokenType::FALSE:
  case TokenType::UNIT:
    return parse_literal();
  case TokenType::IDENTIFIER:
    return parse_non_annotated_identifier();
  case TokenType::LPAREN: {
    expect(TokenType::LPAREN, __FUNCTION__);
    auto expression = parse_expression();
    expect(TokenType::RPAREN, __FUNCTION__);
    return expression;
  }
  case TokenType::LBRACE:
    return std::make_unique<Block>(parse_block());
  case TokenType::IF:
    return parse_if_expr();
  case TokenType::PIPE:
    return parse_lambda_expr();
  case TokenType::RETURN:
    return parse_return_expr();
    // case TokenType::WHILE:
    // case TokenType::FOR:
  default:
    on_error(peek().get_location(),
             "Error parsing primary, Unexpected TokenType:",
             peek().get_token_type());
  }
}

// --- Compound expressions --------------------------------------------------

Expression Parser::parse_if_expr() {
  auto original_location = peek().get_location();
  expect(TokenType::IF, __FUNCTION__);

  auto condition = parse_expression();

  auto then_branch = parse_block();

  if (peek().get_token_type() == TokenType::ELSE) {
    advance();
    return std::make_unique<IfExpr>(
        IfExpr{{original_location},
               std::move(condition),
               std::move(then_branch),
               std::optional<Block>(parse_block())});
  }

  return std::make_unique<IfExpr>(IfExpr{{original_location},
                                         std::move(condition),
                                         std::move(then_branch),
                                         std::nullopt});
}

Expression Parser::parse_return_expr() {
  auto original_location = peek().get_location();
  expect(TokenType::RETURN, __FUNCTION__);
  return std::make_unique<ReturnExpr>(
      ReturnExpr{{original_location}, parse_expression()});
}

Expression Parser::parse_lambda_expr() {
  // TODO
  on_error(peek().get_location(), "parse_lambda_expr not implemented");
}

Expression Parser::parse_literal() {
  switch (peek().get_token_type()) {
  case TokenType::TRUE:
    return LiteralBool{{advance()->get_location()}, true};
  case TokenType::FALSE:
    return LiteralBool{{advance()->get_location()}, false};
  case TokenType::UNIT:
    return LiteralUnit{{advance()->get_location()}};
  case TokenType::INT_LITERAL: {
    const auto token = advance();
    const auto *int_token_ptr = dynamic_cast<const TokenNumber *>(token.get());
    if (int_token_ptr == nullptr) {
      on_error(token->get_location(), "Error casting token to TokenNumber");
    }
    try {
      return LiteralInt64{{int_token_ptr->get_location()},
                          std::stoll(int_token_ptr->get_value())};
    } catch (std::out_of_range &error) {
      on_error(int_token_ptr->get_location(),
               "Could not cast TokenNumber with lexeme: \"",
               int_token_ptr->get_value(), "\" to int64: ", error.what());
    }
  }
  default:
    on_error(peek().get_location(),
             "Unexpected Token for parse_literal of TokenType: ",
             peek().get_token_type());
  }
}

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
