//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Source file for BlipAST class
//*
//*
//****************************************************************************

#include <iostream>                                      // For cout
#include <blipast.hpp>                                   // For blip class
#include <sc/token.hpp>                                  // For Token class

//****************************************************************************
namespace blip {
//****************************************************************************
using namespace sc;

BlipAST::BlipAST() {}

BlipAST::~BlipAST() {}

void BlipAST::PrettyPrint() {
  std::cout << "TODO: PrettyPrint the Blip AST" << std::endl;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
