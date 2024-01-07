//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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
#include <string>

#include <sc/sc.hpp>
#include <sc/source_location.hpp> // For SourceLocation classes

//****************************************************************************
namespace sc {
//****************************************************************************

class LexerException : public std::runtime_error {
public:
  LexerException(const std::string &message, const SourceLocation &);
};

//****************************************************************************
} // namespace sc
//****************************************************************************
