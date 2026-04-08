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

#include "codegen/basic_block.hpp"
#include "codegen/function.hpp"
#include "codegen/module.hpp"
#include "codegen/symbol_table.hpp"
#include <cassert>
#include <memory>
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Function &current_function() { return m_module.current_function(); }

  BasicBlock &current_basic_block() {
    return current_function().current_basic_block();
  }

  std::string m_output{};
  Module m_module{};
  SymbolTable m_symbol_table;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
