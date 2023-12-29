//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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

#include <sc/exceptions.hpp>      // For exception classes
#include <sc/sc.hpp>              // For forward decls
#include <sc/source_location.hpp> // For SourceLocation classes

//****************************************************************************
namespace sc {
//****************************************************************************

std::string construct_message(const std::string &message,
                              const SourceLocation &loc) {
  std::stringstream s;
  s << "LexerException: ";
  loc.dump(s);
  s << message;
  return s.str();
}

LexerException::LexerException(const std::string &message,
                               const SourceLocation &loc)
    : std::runtime_error(construct_message(message, loc)) {}

//****************************************************************************
} // namespace sc
//****************************************************************************
