//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : BlipAST class to represent the abstract syntax tree of the blip
//*            language
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <vector>
#include <blipast.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class BlipAST {
public:

  BlipAST();

  ~BlipAST();

  void PrettyPrint();

private:
  
};

//****************************************************************************
} // namespace blip
//****************************************************************************
