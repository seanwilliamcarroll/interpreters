//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
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

#include <iosfwd>
#include <memory>

#include <blip_tokens.hpp>
#include <parser.hpp>
#include <sc/lexer_interface.hpp>
#include <sc/sc.hpp>
#include <sc/token.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Blip {
public:
  Blip(std::istream &, const char *hint = "<input>");

  // Read Eval Print
  void rep();

private:
  std::unique_ptr<Parser> m_parser;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
