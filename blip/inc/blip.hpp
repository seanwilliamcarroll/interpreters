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

#include "environment.hpp"
#include <iosfwd>
#include <memory>

#include <parser.hpp>

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
  std::shared_ptr<ValueEnvironment> m_top_value_env;
  std::shared_ptr<TypeEnvironment> m_top_type_env;
  // Need to keep old programs around if we want persistence during the repl
  std::vector<std::unique_ptr<ProgramNode>> m_past_programs;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
