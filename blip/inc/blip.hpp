//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : blip::Blip
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Blip {
public:
  Blip(std::istream &, const char *hint = "<input>");

  // Read Eval Print
  void rep();

private:
  std::unique_ptr<sc::LexerInterface> m_lexer;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
