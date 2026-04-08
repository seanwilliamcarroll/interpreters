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
#include <memory>
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Module &new_module() {
    m_modules.emplace_back(std::make_unique<Module>());
    return current_module();
  }

  Module &current_module() { return *m_modules.back(); }

  Function &current_function() { return current_module().current_function(); }

  BasicBlock &current_basic_block() {
    return current_function().current_basic_block();
  }

  std::string m_output{};
  std::vector<std::unique_ptr<Module>> m_modules;
  SymbolTable m_symbol_table;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
