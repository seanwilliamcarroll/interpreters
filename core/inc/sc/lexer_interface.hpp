//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LexerInterface abstract interface
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>

#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************


class LexerInterface {
public:

  virtual std::unique_ptr<Token> getNextToken() = 0;

};
  
  
//****************************************************************************
} // namespace sc
//****************************************************************************
