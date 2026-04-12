//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/type_registry.hpp"
#include <cassert>
#include <codegen/basic_block.hpp>
#include <codegen/function.hpp>
#include <codegen/module.hpp>
#include <codegen/symbol_table.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Context(const hir::TypeRegistry &type_registry)
      : m_type_registry(type_registry) {}

  Module &module() { return m_module; }
  SymbolTable &symbols() { return m_symbol_table; }
  Function &function() { return m_module.current_function(); }
  BasicBlock &block() { return function().current_basic_block(); }

  const hir::TypeRegistry &type_registry() const { return m_type_registry; }

private:
  Module m_module{};
  SymbolTable m_symbol_table{};
  const hir::TypeRegistry &m_type_registry;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
