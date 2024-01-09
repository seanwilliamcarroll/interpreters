//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Blip
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <iosfwd>
#include <memory>

#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Blip {
public:
  enum BlipTokenType : sc::TokenType {
    IF = sc::Token::END_TOKEN + 1,
    WHILE,
    SET,
    BEGIN,
    PRINT,
    DEFINE,

    START_TOKEN = IF,
    END_TOKEN = DEFINE
  };

  Blip(std::istream &, const char *hint = "<input>");

  // Read Eval Print
  void rep();

private:
  std::unique_ptr<sc::LexerInterface> m_lexer;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
