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

#include "sc/ast.hpp"
#include "sc/exceptions.hpp"
#include "sc/source_location.hpp"
#include <iosfwd>
#include <memory>
#include <sstream>

#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Parser {
public:
  // Reasonable to assume the parser should own its lexer
  Parser(std::unique_ptr<sc::LexerInterface> lexer)
      : m_lexer(std::move(lexer)), m_current_token() {}

  virtual ~Parser() = default;

  std::unique_ptr<sc::AstNode> parse();

private:
  std::unique_ptr<sc::AstNode> parse_expression();
  const sc::Token &peek();

  std::unique_ptr<sc::Token> advance();

  std::unique_ptr<sc::Token> expect(sc::TokenType);

  template <class... args>
  [[noreturn]] void on_error(const sc::SourceLocation &location,
                             args &&...a) const {

    std::ostringstream o; // Local stringstream

    (o << ... << a); // Fold operator <<

    throw sc::ParserException(o.str(), location);
  }

  const std::unique_ptr<sc::LexerInterface> m_lexer;
  std::unique_ptr<sc::Token> m_current_token;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
