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

std::string construct_message(std::string_view message,
                              const SourceLocation &loc) {
  std::stringstream s;
  s << "CompilerException: " << loc << " " << message;
  return s.str();
}

CompilerException::CompilerException(std::string_view message,
                                     const SourceLocation &loc)
    : std::runtime_error(construct_message(message, loc)) {}

//****************************************************************************
} // namespace sc
//****************************************************************************
