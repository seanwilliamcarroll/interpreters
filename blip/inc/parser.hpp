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

  std::unique_ptr<sc::AstNode> parse() {
    return {};
  }
  
private:
  const sc::Token &peek() {
    if (m_current_token == nullptr) {
      m_current_token = m_lexer->get_next_token();
    }
    return *m_current_token;
  }

  std::unique_ptr<sc::Token> advance() {
    if (m_current_token == nullptr) {
      m_current_token = m_lexer->get_next_token();
    }
    auto output = std::move(m_current_token);
    m_current_token = nullptr;
    return output;
  }

  std::unique_ptr<sc::Token> expect(sc::TokenType token_type) {
    const auto &current_token = peek();
    if (current_token.get_token_type() != token_type) {
      on_error(current_token.get_location(),
               "Unexpected Token of type: ", current_token.get_token_type());
    }
    return advance();
  }

  template <class... args>
  [[noreturn]] void on_error(const sc::SourceLocation &location, args &&...a) const {

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
