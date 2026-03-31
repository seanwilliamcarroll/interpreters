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
  std::shared_ptr<ValueEnvironment> m_top_env;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
