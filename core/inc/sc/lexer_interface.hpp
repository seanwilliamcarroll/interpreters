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

//****************************************************************************
namespace sc {
//****************************************************************************

struct LexerInterface {
  virtual ~LexerInterface() = default;
  virtual std::unique_ptr<Token> get_next_token() = 0;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
