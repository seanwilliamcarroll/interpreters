//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : bust::Parser implementation
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <ast/types.hpp>
#include <lexer_interface.hpp>
#include <operators.hpp>
#include <parser.hpp>
#include <source_location.hpp>
#include <token.hpp>
#include <tokens.hpp>
#include <types.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************

constexpr size_t HEXADECIMAL_BASE_16 = 16ULL;

// --- Public ----------------------------------------------------------------

ast::Program Parser::parse() { return parse_program(); }

// --- Top-level -------------------------------------------------------------

ast::Program Parser::parse_program() {

  auto starting_location = peek().get_location();

  std::vector<ast::TopItem> top_items;

  while (peek().get_token_type() != TokenType::EOF_TOKEN) {
    top_items.push_back(parse_top_item());
  }

  return {{.m_location = starting_location}, std::move(top_items)};
}

ast::TopItem Parser::parse_top_item() {
  if (peek().get_token_type() == TokenType::FN) {
    return parse_func_def();
  }
  if (peek().get_token_type() == TokenType::EXTERN) {
    return parse_extern_func_declaration();
  }
  if (peek().get_token_type() == TokenType::LET) {
    return parse_let_binding();
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

std::unique_ptr<ast::FunctionTypeIdentifier>
Parser::parse_function_type_identifier() {
  auto original_location = peek().get_location();
  expect(TokenType::FN, __FUNCTION__);

  std::vector<ast::TypeIdentifier> argument_types;
  if (peek().get_token_type() == TokenType::LPAREN) {
    expect(TokenType::LPAREN, __FUNCTION__);

    while (peek().get_token_type() != TokenType::RPAREN) {
      argument_types.push_back(parse_type_identifier());
      if (peek().get_token_type() != TokenType::RPAREN) {
        expect(TokenType::COMMA, __FUNCTION__);
      }
    }

    expect(TokenType::RPAREN, __FUNCTION__);
  } else {
    expect(TokenType::UNIT, __FUNCTION__);
  }

  expect(TokenType::ARROW, __FUNCTION__);

  auto return_type = parse_type_identifier();

  return std::make_unique<ast::FunctionTypeIdentifier>(
      ast::FunctionTypeIdentifier{{original_location},
                                  std::move(argument_types),
                                  std::move(return_type)});
}

ast::TypeIdentifier Parser::parse_type_identifier() {
  switch (peek().get_token_type()) {
  case TokenType::UNIT:
    return ast::PrimitiveTypeIdentifier{{advance()->get_location()},
                                        PrimitiveType::UNIT};
  case TokenType::BOOL:
    return ast::PrimitiveTypeIdentifier{{advance()->get_location()},
                                        PrimitiveType::BOOL};
  case TokenType::CHAR:
    return ast::PrimitiveTypeIdentifier{{advance()->get_location()},
                                        PrimitiveType::CHAR};
  case TokenType::I8:
    return ast::PrimitiveTypeIdentifier{{advance()->get_location()},
                                        PrimitiveType::I8};
  case TokenType::I32:
    return ast::PrimitiveTypeIdentifier{{advance()->get_location()},
                                        PrimitiveType::I32};
  case TokenType::I64:
    return ast::PrimitiveTypeIdentifier{{advance()->get_location()},
                                        PrimitiveType::I64};
  case TokenType::FN:
    return parse_function_type_identifier();
  default:
    const auto [location, identifier_name] =
        parse_location_name_from_identifier("Malformed type annotation");

    return ast::DefinedType{
        {location},
        identifier_name,
    };
  }
}

ast::TypeIdentifier Parser::parse_type_annotation() {
  expect(TokenType::COLON, __FUNCTION__);
  return parse_type_identifier();
}

ast::TypeIdentifier Parser::parse_function_return_type() {
  expect(TokenType::ARROW, __FUNCTION__);
  return parse_type_identifier();
}

ast::Identifier Parser::parse_non_annotated_identifier() {
  const auto [location, identifier_name] =
      parse_location_name_from_identifier("Malformed parameter");

  if (peek().get_token_type() == TokenType::COLON) {
    on_error(peek().get_location(),
             "Expected Identifier without Type Annotation here!");
  }

  return {{location}, identifier_name, std::nullopt};
}

ast::Identifier Parser::parse_possibly_annotated_identifier() {
  const auto [location, identifier_name] =
      parse_location_name_from_identifier("Malformed parameter");

  auto type = (peek().get_token_type() == TokenType::COLON)
                  ? std::optional(parse_type_annotation())
                  : std::nullopt;

  return {{location}, identifier_name, std::move(type)};
}

ast::Identifier Parser::parse_annotated_identifier() {
  const auto [location, identifier_name] =
      parse_location_name_from_identifier("Malformed parameter");

  auto type = parse_type_annotation();

  return {{location}, identifier_name, std::move(type)};
}

ast::FunctionDeclaration Parser::parse_function_declaration() {
  expect(TokenType::FN, __FUNCTION__);

  auto function_id = parse_non_annotated_identifier();

  std::vector<ast::Identifier> parameters{};
  if (peek().get_token_type() == TokenType::UNIT) {
    advance();
  } else {
    expect(TokenType::LPAREN, __FUNCTION__);

    parameters = parse_param_list();

    expect(TokenType::RPAREN, __FUNCTION__);
  }

  auto return_type = (peek().get_token_type() == TokenType::ARROW)
                         ? parse_function_return_type()
                         : ast::PrimitiveTypeIdentifier{
                               {function_id.m_location},
                               PrimitiveType::UNIT,
                           };

  return {.m_id = std::move(function_id),
          .m_parameters = std::move(parameters),
          .m_return_type = std::move(return_type)};
}

ast::FunctionDef Parser::parse_func_def() {
  auto original_location = peek().get_location();
  auto signature = parse_function_declaration();

  auto body = parse_block();

  return {{original_location}, std::move(signature), std::move(body)};
}

ast::ExternFunctionDeclaration Parser::parse_extern_func_declaration() {
  auto original_location = peek().get_location();
  expect(TokenType::EXTERN, __FUNCTION__);
  auto signature = parse_function_declaration();
  expect(TokenType::SEMICOLON, __FUNCTION__);

  return {{original_location}, std::move(signature)};
}

ast::LetBinding Parser::parse_let_binding() {
  auto original_location = peek().get_location();
  expect(TokenType::LET, __FUNCTION__);

  auto identifier = parse_possibly_annotated_identifier();

  expect(TokenType::EQUALS, __FUNCTION__);

  auto body = parse_expression();

  expect(TokenType::SEMICOLON, __FUNCTION__);

  return {{original_location}, std::move(identifier), std::move(body)};
}

std::vector<ast::Identifier> Parser::parse_param_list() {
  std::vector<ast::Identifier> parameters;
  while (peek().get_token_type() != TokenType::RPAREN) {
    parameters.push_back(parse_annotated_identifier());
    if (peek().get_token_type() != TokenType::RPAREN) {
      expect(TokenType::COMMA, __FUNCTION__);
    }
  }
  return parameters;
}

std::vector<ast::Identifier> Parser::parse_lambda_param_list() {
  std::vector<ast::Identifier> parameters;
  while (peek().get_token_type() != TokenType::PIPE) {
    // Could force all or nothing, either they all have an annotation or none of
    // them
    // Flexible for now
    parameters.push_back(parse_possibly_annotated_identifier());
    if (peek().get_token_type() != TokenType::PIPE) {
      expect(TokenType::COMMA, __FUNCTION__);
    }
  }
  return parameters;
}

// --- Blocks ----------------------------------------------------------------

bool is_expression_no_semicolon(const ast::Expression &expression) {
  return std::holds_alternative<std::unique_ptr<ast::IfExpr>>(
             expression.m_expression) ||
         std::holds_alternative<std::unique_ptr<ast::Block>>(
             expression.m_expression) ||
         std::holds_alternative<std::unique_ptr<ast::WhileExpr>>(
             expression.m_expression) ||
         std::holds_alternative<std::unique_ptr<ast::ForExpr>>(
             expression.m_expression);
}

ast::Block Parser::parse_block() {
  auto original_location = peek().get_location();
  expect(TokenType::LBRACE, __FUNCTION__);

  std::vector<ast::Statement> statements;
  while (peek().get_token_type() != TokenType::RBRACE) {
    if (peek().get_token_type() == TokenType::LET) {
      auto let_binding = parse_let_binding();
      statements.emplace_back(std::move(let_binding));
      continue;
    }
    auto next_expression = parse_expression();
    if (peek().get_token_type() == TokenType::SEMICOLON) {
      expect(TokenType::SEMICOLON, __FUNCTION__);
      statements.emplace_back(std::move(next_expression));
      continue;
    }
    if (is_expression_no_semicolon(next_expression) &&
        peek().get_token_type() != TokenType::RBRACE) {
      // Still more statements to come then
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

ast::Expression Parser::parse_expression() { return parse_logic_or(); }

ast::Expression Parser::parse_binary_expression(
    const std::unordered_map<TokenType, BinaryOperator> &possible_mappings,
    auto next_clause_to_parse) {
  auto original_location = peek().get_location();

  auto expression = next_clause_to_parse();

  auto iter = possible_mappings.find(peek().get_token_type());

  while (iter != possible_mappings.end()) {
    auto op = iter->second;
    advance();
    expression = {{{original_location}},
                  std::make_unique<ast::BinaryExpr>(ast::BinaryExpr{
                      op, std::move(expression), next_clause_to_parse()})};
    iter = possible_mappings.find(peek().get_token_type());
  }

  return expression;
}

ast::Expression Parser::parse_logic_or() {
  return parse_binary_expression(
      {{TokenType::OR_OR, BinaryOperator::LOGICAL_OR}},
      [this] { return parse_logic_and(); });
}

ast::Expression Parser::parse_logic_and() {
  return parse_binary_expression(
      {{TokenType::AND_AND, BinaryOperator::LOGICAL_AND}},
      [this] { return parse_equality(); });
}

ast::Expression Parser::parse_equality() {
  return parse_binary_expression(
      {
          {TokenType::EQ_EQ, BinaryOperator::EQ},
          {TokenType::BANG_EQ, BinaryOperator::NOT_EQ},
      },
      [this] { return parse_comparison(); });
}

ast::Expression Parser::parse_comparison() {
  return parse_binary_expression(
      {
          {TokenType::LESS, BinaryOperator::LT},
          {TokenType::GREATER, BinaryOperator::GT},
          {TokenType::LESS_EQ, BinaryOperator::LT_EQ},
          {TokenType::GREATER_EQ, BinaryOperator::GT_EQ},
      },
      [this] { return parse_add_sub(); });
}

ast::Expression Parser::parse_add_sub() {
  return parse_binary_expression(
      {
          {TokenType::PLUS, BinaryOperator::PLUS},
          {TokenType::MINUS, BinaryOperator::MINUS},
      },
      [this] { return parse_mult_div_mod(); });
}

ast::Expression Parser::parse_mult_div_mod() {
  return parse_binary_expression(
      {
          {TokenType::STAR, BinaryOperator::MULTIPLIES},
          {TokenType::SLASH, BinaryOperator::DIVIDES},
          {TokenType::PERCENT, BinaryOperator::MODULUS},
      },
      [this] { return parse_unary_pre(); });
}

ast::Expression Parser::parse_unary_pre() {
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
    return {{original_location},
            std::make_unique<ast::UnaryExpr>(
                ast::UnaryExpr{op, parse_cast_expr()})};
  }

  return parse_cast_expr();
}

ast::Expression Parser::parse_cast_expr() {
  auto original_location = peek().get_location();

  auto postfix = parse_postfix();

  while (peek().get_token_type() == TokenType::AS) {
    advance();

    auto casting_type = parse_type_identifier();

    postfix =
        ast::Expression{{original_location},
                        std::make_unique<ast::CastExpr>(ast::CastExpr{
                            std::move(postfix), std::move(casting_type)})};
  }

  return postfix;
}

ast::Expression Parser::parse_postfix() {
  auto original_location = peek().get_location();

  auto expression = parse_primary();

  while (peek().get_token_type() == TokenType::LPAREN ||
         peek().get_token_type() == TokenType::UNIT) {
    if (peek().get_token_type() == TokenType::UNIT) {
      advance();
      expression =
          ast::Expression{{original_location},
                          std::make_unique<ast::CallExpr>(
                              ast::CallExpr{std::move(expression), {}})};
      continue;
    }

    expect(TokenType::LPAREN, __FUNCTION__);

    std::vector<ast::Expression> arguments;
    while (peek().get_token_type() != TokenType::RPAREN) {
      arguments.push_back(parse_expression());

      if (peek().get_token_type() != TokenType::RPAREN) {
        expect(TokenType::COMMA, __FUNCTION__);
      }
    }

    expect(TokenType::RPAREN, __FUNCTION__);
    expression =
        ast::Expression{{original_location},
                        std::make_unique<ast::CallExpr>(ast::CallExpr{
                            std::move(expression), std::move(arguments)})};
  }

  return expression;
}

ast::Expression Parser::parse_primary() {
  switch (peek().get_token_type()) {
  case TokenType::INT_LITERAL:
  case TokenType::CHAR_LITERAL:
  case TokenType::TRUE:
  case TokenType::FALSE:
  case TokenType::UNIT:
    return parse_literal();
  case TokenType::IDENTIFIER: {
    auto identifier = parse_non_annotated_identifier();
    return {{identifier.m_location}, std::move(identifier)};
  }
  case TokenType::LPAREN: {
    expect(TokenType::LPAREN, __FUNCTION__);
    auto expression = parse_expression();
    expect(TokenType::RPAREN, __FUNCTION__);
    return expression;
  }
  case TokenType::LBRACE: {
    auto block = std::make_unique<ast::Block>(parse_block());
    return {{block->m_location}, std::move(block)};
  }
  case TokenType::IF:
    return parse_if_expr();
  case TokenType::PIPE:
  case TokenType::OR_OR:
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

ast::Expression Parser::parse_if_expr() {
  auto original_location = peek().get_location();
  expect(TokenType::IF, __FUNCTION__);

  auto condition = parse_expression();

  auto then_block = parse_block();

  if (peek().get_token_type() == TokenType::ELSE) {
    expect(TokenType::ELSE, __FUNCTION__);
    auto else_block = std::optional<ast::Block>(parse_block());

    return ast::Expression{{original_location},
                           std::make_unique<ast::IfExpr>(ast::IfExpr{
                               std::move(condition), std::move(then_block),
                               std::move(else_block)})};
  }

  return ast::Expression{
      {original_location},
      std::make_unique<ast::IfExpr>(ast::IfExpr{
          std::move(condition), std::move(then_block), std::nullopt})};
}

ast::Expression Parser::parse_return_expr() {
  auto original_location = peek().get_location();
  expect(TokenType::RETURN, __FUNCTION__);
  return ast::Expression{
      {original_location},
      std::make_unique<ast::ReturnExpr>(ast::ReturnExpr{parse_expression()})};
}

ast::Expression Parser::parse_lambda_expr() {
  auto original_location = peek().get_location();
  std::vector<ast::Identifier> arguments;
  if (peek().get_token_type() == TokenType::PIPE) {
    // May have arguments
    expect(TokenType::PIPE, __FUNCTION__);
    arguments = parse_lambda_param_list();
    expect(TokenType::PIPE, __FUNCTION__);
  } else if (peek().get_token_type() == TokenType::OR_OR) {
    // Definitely no arguments
    expect(TokenType::OR_OR, __FUNCTION__);
  } else {
    on_error(peek().get_location(),
             "parse_lambda_expr did not expect TokenType: ",
             peek().get_token_type());
  }

  auto function_type =
      (peek().get_token_type() == TokenType::ARROW)
          ? std::optional<ast::TypeIdentifier>(parse_function_return_type())
          : std::nullopt;

  auto body = parse_block();

  return ast::Expression{{original_location},
                         std::make_unique<ast::LambdaExpr>(ast::LambdaExpr{
                             std::move(arguments),

                             std::move(body), std::move(function_type)})};
}

ast::Expression Parser::parse_literal() {
  const auto original_location = peek().get_location();
  switch (peek().get_token_type()) {
  case TokenType::TRUE:
    return {{advance()->get_location()}, ast::LiteralBool{true}};
  case TokenType::FALSE:
    return {{advance()->get_location()}, ast::LiteralBool{false}};
  case TokenType::UNIT:
    return {{advance()->get_location()}, ast::LiteralUnit{}};
  case TokenType::INT_LITERAL: {
    const auto token = advance();
    const auto *int_token_ptr = dynamic_cast<const TokenNumber *>(token.get());
    if (int_token_ptr == nullptr) {
      on_error(token->get_location(), "Error casting token to TokenNumber");
    }
    try {
      return {{original_location},
              ast::LiteralI64{std::stoll(int_token_ptr->get_value())}};
    } catch (std::out_of_range &error) {
      on_error(int_token_ptr->get_location(),
               "Could not cast TokenNumber with lexeme: \"",
               int_token_ptr->get_value(), "\" to int64: ", error.what());
    }
  }
  case TokenType::CHAR_LITERAL: {
    const auto token = advance();
    const auto *char_token_ptr = dynamic_cast<const TokenChar *>(token.get());
    if (char_token_ptr == nullptr) {
      on_error(token->get_location(), "Error casting token to TokenChar");
    }
    // We expect we've lexed things correctly, so should be a valid char
    const auto &lexeme = char_token_ptr->get_value();
    if (lexeme[1] == '\\') {
      // Escaped char
      const static std::unordered_map<char, char> escaped_chars{
          {'n', '\n'},  {'t', '\t'},  {'r', '\r'},
          {'\\', '\\'}, {'\'', '\''}, {'0', '\0'},
      };
      auto iter = escaped_chars.find(lexeme[2]);
      if (iter != escaped_chars.end()) {
        return {{original_location}, ast::LiteralChar{iter->second}};
      }
      // Must be \x, want indices 3 and 4
      return {{original_location},
              ast::LiteralChar{static_cast<char>(std::stoi(
                  lexeme.substr(3, 2), nullptr, HEXADECIMAL_BASE_16))}};
    }
    // Must have been printable
    return {{original_location}, ast::LiteralChar{lexeme[1]}};
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
