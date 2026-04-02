//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Lexer implementation for bust.
//*
//*
//****************************************************************************

#include <iostream>
#include <memory>

#include <lexer.hpp>
#include <source_location.hpp>

//****************************************************************************
namespace bust {
namespace {
//****************************************************************************

class Lexer : public LexerInterface {
public:
  Lexer(std::istream &input, const char *filename)
      : m_input(input), m_filename(filename) {}

  std::unique_ptr<Token> get_next_token() override {
    // TODO: implement lexing
    (void)m_input;
    return std::make_unique<Token>(
        core::SourceLocation(m_filename, m_line, m_column),
        TokenType::EOF_TOKEN);
  }

private:
  std::istream &m_input;
  const char *m_filename;
  unsigned int m_line = 1;
  unsigned int m_column = 0;
};

//****************************************************************************
} // anonymous namespace
//****************************************************************************

std::unique_ptr<LexerInterface> make_lexer(std::istream &input,
                                           const char *hint) {
  return std::make_unique<Lexer>(input, hint);
}

//****************************************************************************
} // namespace bust
//****************************************************************************
