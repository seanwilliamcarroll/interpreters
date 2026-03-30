//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : source file for exceptions header file
//*
//*
//****************************************************************************

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <sc/exceptions.hpp>      // For exception classes
#include <sc/sc.hpp>              // For forward decls
#include <sc/source_location.hpp> // For SourceLocation classes

//****************************************************************************
namespace sc {
//****************************************************************************

std::string construct_message(const char* exception_type, std::string_view message,
                              const SourceLocation &loc) {
  std::stringstream s;
  s << exception_type << ": " << loc << " " << message;
  return s.str();
}

LexerException::LexerException(std::string_view message,
                                     const SourceLocation &loc)
  : std::runtime_error(construct_message("LexerException", message, loc)) {}

ParserException::ParserException(std::string_view message,
                                     const SourceLocation &loc)
  : std::runtime_error(construct_message("ParserException", message, loc)) {}

//****************************************************************************
} // namespace sc
//****************************************************************************
