//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LanguageInterface abstract interface
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <iostream>
#include <memory>
#include <string>

#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

class LanguageInterface {
public:
  // FIXME: These functions should have arguments and return values eventually
  virtual void parse() = 0;
  virtual void eval() = 0;

protected:
  LanguageInterface(std::unique_ptr<LexerInterface> lexer);
  virtual std::unique_ptr<LexerInterface>
  construct_lexer(std::istream &in_stream,
                  const std::string &file_name = "<input>") = 0;

  std::unique_ptr<LexerInterface> m_lexer;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
