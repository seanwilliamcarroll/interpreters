//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Source file for Blip class
//*
//*
//****************************************************************************

#include <vector>                                        // For vector
#include <blip.hpp>                                      // For Blip class
#include <blipast.hpp>                                   // For BlipAST class
#include <sc/token.hpp>                                  // For Token class

//****************************************************************************
namespace blip {
//****************************************************************************
using namespace sc;

Blip::Blip() {}

Blip::~Blip() {}

BlipAST Blip::Parse(const std::vector<Token> tokens) {

  // TODO: Parse tokens and construct Blip AST
  
  return BlipAST();
}


//****************************************************************************
} // namespace blip
//****************************************************************************
