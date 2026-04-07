//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Shared context for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/symbol_table.hpp"
#include <cassert>
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {

  std::string m_output{};
  SymbolTable m_symbol_table;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
