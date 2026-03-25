//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Exception classes
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <stdexcept>
#include <string_view>

#include <sc/sc.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

struct LexerException : std::runtime_error {
  // FIXME: Check on use of string_view
  //        Decide where to construct the message itself

  LexerException(std::string_view message, const SourceLocation &);
};

struct ParserException : std::runtime_error {
  // FIXME: Check on use of string_view
  //        Decide where to construct the message itself

  ParserException(std::string_view message, const SourceLocation &);
};

struct UnknownTokenTypeException : std::runtime_error {
  using std::runtime_error::runtime_error;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
