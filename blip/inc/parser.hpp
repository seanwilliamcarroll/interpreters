//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Parser
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <iosfwd>
#include <memory>

#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Parser {
public:

  Parser() {}

  virtual ~Parser() = default;


  // SWC: Parser should probably have access to lexer, so it can pull tokens out

  
private:

};

//****************************************************************************
} // namespace blip
//****************************************************************************
