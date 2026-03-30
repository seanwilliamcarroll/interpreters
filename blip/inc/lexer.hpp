//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Lexer factory function
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <initializer_list>
#include <iosfwd>
#include <memory>

#include <blip_tokens.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

std::unique_ptr<LexerInterface>
make_lexer(std::istream &, std::initializer_list<Keyword>,
           const char *hint = "<input>");

//****************************************************************************
} // namespace blip
//****************************************************************************
