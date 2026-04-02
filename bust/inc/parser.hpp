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
#include <sstream>

#include <ast.hpp>
#include <bust_tokens.hpp>
#include <exceptions.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

class Parser {
public:
  explicit Parser(std::unique_ptr<LexerInterface> lexer)
      : m_lexer(std::move(lexer)), m_current_token() {}

  virtual ~Parser() = default;

  void reset() { m_current_token = nullptr; }

  Program parse();

private:
  Program parse_program();
  TopItem parse_top_item();
  FunctionDef parse_func_def();
  LetBinding parse_let_binding();
  std::vector<Identifier> parse_param_list();
  TypeAnnotation parse_type();

  Block parse_block();

  Expression parse_expression();
  Expression parse_logic_or();
  Expression parse_logic_and();
  Expression parse_equality();
  Expression parse_comparison();
  Expression parse_add_sub();
  Expression parse_mult_div_mod();
  Expression parse_unary_pre();
  Expression parse_postfix();
  Expression parse_primary();

  Expression parse_if_expr();
  Expression parse_return_expr();
  Expression parse_lambda_expr();
  Expression parse_literal();

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
