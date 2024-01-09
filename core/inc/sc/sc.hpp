//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
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
#include <memory>
#include <string_view>

//****************************************************************************
namespace sc {
//****************************************************************************

struct CompilerException;

struct SourceLocation;

using TokenType = unsigned int;

class Token;

template <typename> class TokenOf;

struct LexerInterface;

using Keyword = std::pair<const std::string_view, TokenType>;

std::unique_ptr<LexerInterface> make_lexer(std::istream &,
                                           std::initializer_list<Keyword>,
                                           const char *hint = "<input>");

//****************************************************************************
} // namespace sc
//****************************************************************************
