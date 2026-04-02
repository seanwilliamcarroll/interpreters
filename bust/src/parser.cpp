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

// --- Public ----------------------------------------------------------------

Program Parser::parse() { return parse_program(); }

// --- Top-level -------------------------------------------------------------

Program Parser::parse_program() {
  // TODO
  return Program{};
}

TopItem Parser::parse_top_item() {
  // TODO: peek for FN or LET
  on_error(peek().get_location(), "parse_top_item not implemented");
}

FunctionDef Parser::parse_func_def() {
  // TODO
  on_error(peek().get_location(), "parse_func_def not implemented");
}

LetBinding Parser::parse_let_binding() {
  // TODO
  on_error(peek().get_location(), "parse_let_binding not implemented");
}

std::vector<Identifier> Parser::parse_param_list() {
  // TODO
  on_error(peek().get_location(), "parse_param_list not implemented");
}

TypeAnnotation Parser::parse_type() {
  // TODO
  on_error(peek().get_location(), "parse_type not implemented");
}

// --- Blocks ----------------------------------------------------------------

Block Parser::parse_block() {
  // TODO
  on_error(peek().get_location(), "parse_block not implemented");
}

// --- Expression precedence chain -------------------------------------------

Expression Parser::parse_expression() { return parse_logic_or(); }

Expression Parser::parse_logic_or() {
  // TODO: parse_logic_and, loop on OR_OR
  return parse_logic_and();
}

Expression Parser::parse_logic_and() {
  // TODO: parse_equality, loop on AND_AND
  return parse_equality();
}

Expression Parser::parse_equality() {
  // TODO: parse_comparison, loop on EQ_EQ | BANG_EQ
  return parse_comparison();
}

Expression Parser::parse_comparison() {
  // TODO: parse_add_sub, loop on LESS | GREATER | LESS_EQ | GREATER_EQ
  return parse_add_sub();
}

Expression Parser::parse_add_sub() {
  // TODO: parse_mult_div_mod, loop on PLUS | MINUS
  return parse_mult_div_mod();
}

Expression Parser::parse_mult_div_mod() {
  // TODO: parse_unary_pre, loop on STAR | SLASH | PERCENT
  return parse_unary_pre();
}

Expression Parser::parse_unary_pre() {
  // TODO: check for MINUS | BANG, then parse_postfix
  return parse_postfix();
}

Expression Parser::parse_postfix() {
  // TODO: parse_primary, loop on LPAREN for call expressions
  return parse_primary();
}

Expression Parser::parse_primary() {
  // TODO: dispatch on peek token type
  on_error(peek().get_location(), "parse_primary not implemented");
}

// --- Compound expressions --------------------------------------------------

Expression Parser::parse_if_expr() {
  // TODO
  on_error(peek().get_location(), "parse_if_expr not implemented");
}

Expression Parser::parse_return_expr() {
  // TODO
  on_error(peek().get_location(), "parse_return_expr not implemented");
}

Expression Parser::parse_lambda_expr() {
  // TODO
  on_error(peek().get_location(), "parse_lambda_expr not implemented");
}

Expression Parser::parse_literal() {
  // TODO
  on_error(peek().get_location(), "parse_literal not implemented");
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
