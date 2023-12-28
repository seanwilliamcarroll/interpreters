//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Lexer
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <sc/core_lexer.hpp>
#include <sc/sc.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

namespace blip_token_types {
const sc::TokenType IF = 50;
const sc::TokenType WHILE = 51;
const sc::TokenType SET = 52;
const sc::TokenType BEGIN = 53;
const sc::TokenType PRINT = 54;
} // namespace blip_token_types

class Lexer : public sc::CoreLexer {
public:
  Lexer(std::istream &in_stream, const std::string &file_name);

private:
  sc::KeywordsMap construct_keywords_map();
};

//****************************************************************************
} // namespace blip
//****************************************************************************
