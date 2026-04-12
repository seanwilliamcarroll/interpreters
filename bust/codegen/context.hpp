//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/type_arena.hpp"
#include <cassert>
#include <codegen/basic_block.hpp>
#include <codegen/function.hpp>
#include <codegen/module.hpp>
#include <codegen/symbol_table.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Context(const hir::TypeArena &type_arena) : m_type_arena(type_arena) {}

  Module &module() { return m_module; }
  SymbolTable &symbols() { return m_symbol_table; }
  Function &function() { return m_module.current_function(); }
  BasicBlock &block() { return function().current_basic_block(); }

  const hir::TypeArena &type_arena() const { return m_type_arena; }

private:
  Module m_module{};
  SymbolTable m_symbol_table{};
  const hir::TypeArena &m_type_arena;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
