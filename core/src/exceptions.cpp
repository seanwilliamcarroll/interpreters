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

#include <sstream>
#include <string_view>

#include <sc/exceptions.hpp>      // For CompilerException
#include <sc/source_location.hpp> // For SourceLocation

//****************************************************************************
namespace sc {
//****************************************************************************

CompilerException::CompilerException(const char *phase,
                                     std::string_view message,
                                     const SourceLocation &loc)
    : std::runtime_error([&] {
        std::stringstream s;
        s << phase << ": " << loc << " " << message;
        return s.str();
      }()) {}

//****************************************************************************
} // namespace sc
//****************************************************************************
