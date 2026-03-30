//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Parser
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <sstream>
#include <vector>

#include <ast.hpp>
#include <blip_tokens.hpp>
#include <exceptions.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Parser {
public:
  Parser(std::unique_ptr<LexerInterface> lexer)
      : m_lexer(std::move(lexer)), m_current_token() {}

  virtual ~Parser() = default;

  void reset() { m_current_token = nullptr; }

  std::unique_ptr<ProgramNode> parse();

private:
  std::unique_ptr<AstNode> parse_expression();
  std::unique_ptr<AstNode> parse_list();
  std::vector<std::unique_ptr<Identifier>> parse_identifier_list();

  const Token &peek();

  std::unique_ptr<Token> advance();

  std::unique_ptr<Token> expect(TokenType, const char *function_name);

  template <class... args>
  [[noreturn]] void on_error(const core::SourceLocation &location,
                             args &&...a) const {

    std::ostringstream o; // Local stringstream

    (o << ... << a); // Fold operator <<

    throw core::CompilerException("ParserException", o.str(), location);
  }

  const std::unique_ptr<LexerInterface> m_lexer;
  std::unique_ptr<Token> m_current_token;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
