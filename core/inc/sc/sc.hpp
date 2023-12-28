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

#include <map>
#include <memory>
#include <string>

//****************************************************************************
namespace sc {
//****************************************************************************

struct SourceLocation;

using TokenType = unsigned int;

struct Token;

template <typename T> struct TokenOf;

class LexerInterface;

using KeywordsMap = std::map<std::string, TokenType>;

//****************************************************************************
} // namespace sc
//****************************************************************************
