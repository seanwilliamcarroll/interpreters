//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cassert>
#include <codegen/basic_block.hpp>
#include <codegen/function.hpp>
#include <codegen/module.hpp>
#include <codegen/symbol_table.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {

  Module &module() { return m_module; }
  SymbolTable &symbols() { return m_symbol_table; }
  Function &function() { return m_module.current_function(); }
  BasicBlock &block() { return function().current_basic_block(); }

private:
  Module m_module{};
  SymbolTable m_symbol_table;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
