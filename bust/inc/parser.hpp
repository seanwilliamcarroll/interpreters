//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : bust::Parser
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <source_location.hpp>
#include <sstream>

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <tokens.hpp>
#include <unordered_map>

//****************************************************************************
namespace bust {
//****************************************************************************

class Parser {
public:
  explicit Parser(std::unique_ptr<LexerInterface> lexer)
      : m_lexer(std::move(lexer)), m_current_token() {}

  virtual ~Parser() = default;

  void reset() { m_current_token = nullptr; }

  ast::Program parse();

private:
  ast::Program parse_program();
  ast::TopItem parse_top_item();
  ast::FunctionDef parse_func_def();
  ast::LetBinding parse_let_binding();
  std::vector<ast::Identifier> parse_param_list();
  std::vector<ast::Identifier> parse_lambda_param_list();
  std::pair<core::SourceLocation, std::string>
  parse_location_name_from_identifier(const char *error_message);

  ast::TypeIdentifier parse_type_identifier();
  ast::TypeIdentifier parse_function_return_type();
  ast::TypeIdentifier parse_type_annotation();
  std::unique_ptr<ast::FunctionTypeIdentifier> parse_function_type_identifier();

  ast::Identifier parse_non_annotated_identifier();
  ast::Identifier parse_possibly_annotated_identifier();
  ast::Identifier parse_annotated_identifier();

  ast::Block parse_block();

  ast::Expression parse_binary_expression(
      const std::unordered_map<TokenType, BinaryOperator> &possible_mappings,
      auto next_clause_to_parse);

  ast::Expression parse_expression();
  ast::Expression parse_logic_or();
  ast::Expression parse_logic_and();
  ast::Expression parse_equality();
  ast::Expression parse_comparison();
  ast::Expression parse_add_sub();
  ast::Expression parse_mult_div_mod();
  ast::Expression parse_unary_pre();
  ast::Expression parse_cast_expr();
  ast::Expression parse_postfix();
  ast::Expression parse_primary();

  ast::Expression parse_if_expr();
  ast::Expression parse_return_expr();
  ast::Expression parse_lambda_expr();
  ast::Expression parse_literal();

  const Token &peek();

  std::unique_ptr<Token> advance();

  std::unique_ptr<Token> expect(TokenType type, const char *function_name);

  template <class... args>
  [[noreturn]] void on_error(const core::SourceLocation &location,
                             args &&...a) const {
    std::ostringstream o;
    (o << ... << a);
    throw core::CompilerException("ParserException", o.str(), location);
  }

  const std::unique_ptr<LexerInterface> m_lexer;
  std::unique_ptr<Token> m_current_token;
};

//****************************************************************************
} // namespace bust
//****************************************************************************
