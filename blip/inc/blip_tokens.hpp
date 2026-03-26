//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::BlipTokenType, blip::BlipToken
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

using BlipTokenType = sc::TokenType;

class BlipToken : public sc::Token {
public:
  enum CommonBlipTokenType : BlipTokenType {
    IF = sc::Token::END_TOKEN + 1,
    WHILE,
    SET,
    BEGIN,
    PRINT,
    DEFINE,

    START_TOKEN = IF,
    END_TOKEN = DEFINE
  };
};

using sc::Token;
using sc::TokenBool;
using sc::TokenDouble;
using sc::TokenIdentifier;
using sc::TokenInt;
using sc::TokenString;

//****************************************************************************
} // namespace blip
//****************************************************************************
