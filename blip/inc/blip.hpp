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

#include <lexer.hpp>
#include <sc/language_interface.hpp>
#include <sc/sc.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Blip : public sc::LanguageInterface {
public:
  Blip(std::istream &in_stream, const std::string &file_name = "<input>");

  // FIXME: These functions should have arguments and return values eventually
  virtual void parse() override;
  virtual void eval() override;

  // Read Eval Print
  void rep();

protected:
  virtual std::unique_ptr<sc::LexerInterface>
  construct_lexer(std::istream &in_stream,
                  const std::string &file_name = "<input>") override;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
