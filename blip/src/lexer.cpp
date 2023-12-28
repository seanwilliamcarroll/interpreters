//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Lexer source file
//*
//*
//****************************************************************************

#include <lexer.hpp> // blip::Lexer

//****************************************************************************
namespace blip {
//****************************************************************************

using namespace sc;

Lexer::Lexer(std::istream &in_stream, const std::string &file_name)
    : CoreLexer(in_stream, construct_keywords_map(), file_name) {}

KeywordsMap Lexer::construct_keywords_map() const {
  KeywordsMap keywords = {{"if", blip_token_types::IF},
                          {"while", blip_token_types::WHILE},
                          {"set", blip_token_types::SET},
                          {"begin", blip_token_types::BEGIN},
                          {"print", blip_token_types::PRINT}};
  return keywords;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
