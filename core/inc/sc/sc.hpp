//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Forward declarations of major classes and types
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <initializer_list>
#include <iosfwd>
#include <string_view>

//****************************************************************************
namespace sc {
//****************************************************************************

class LexerException;

struct SourceLocation;

using TokenType = unsigned int;

struct Token;

template <typename> struct TokenOf;

struct LexerInterface;

using Keyword = std::pair<const std::string_view, TokenType>;

std::unique_ptr<LexerInterface> make_lexer(std::istream &,
                                           std::initializer_list<Keyword>,
                                           const char *hint = "<input>");

//****************************************************************************
} // namespace sc
//****************************************************************************
