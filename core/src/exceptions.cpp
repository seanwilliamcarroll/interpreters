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
#include <string>

#include <sc/exceptions.hpp> // For exception classes
#include <sc/sc.hpp>         // For forward decls

//****************************************************************************
namespace sc {
//****************************************************************************

LexerException::LexerException(const std::string &message)
    : std::runtime_error(message) {}

//****************************************************************************
} // namespace sc
//****************************************************************************
