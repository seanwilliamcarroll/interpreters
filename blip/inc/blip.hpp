//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Top level Blip class
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <vector>
#include <sc/token.hpp>
#include <blipast.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Blip {
public:

  Blip();

  ~Blip();

  BlipAST Parse(const std::vector<sc::Token> tokens);
  
private:
};

//****************************************************************************
} // namespace blip
//****************************************************************************
