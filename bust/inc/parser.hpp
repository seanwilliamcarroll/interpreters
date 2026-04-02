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

private:
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
