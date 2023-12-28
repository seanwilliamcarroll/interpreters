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

//****************************************************************************
namespace sc {
//****************************************************************************

class LexerException : public std::runtime_error {
public:
  LexerException(const std::string &message);

private:
};

//****************************************************************************
} // namespace sc
//****************************************************************************
