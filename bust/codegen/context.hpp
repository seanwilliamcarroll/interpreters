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
#include "codegen/symbol_table.hpp"
#include <cassert>
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Function &new_function() {
    m_top_level_functions.emplace_back();
    return current_func();
  }

  Function &current_func() { return m_top_level_functions.back(); }

  void add_instruction(Instruction instruction) {
    current_func().add_instruction(std::move(instruction));
  }

  void add_terminal(Terminator terminator) {
    current_func().add_terminal(std::move(terminator));
  }

  std::string m_output{};
  std::vector<Function> m_top_level_functions;
  // How to deal with let bindings at global scope?
  SymbolTable m_symbol_table;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
