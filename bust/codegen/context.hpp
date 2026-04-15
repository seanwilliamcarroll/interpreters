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
#include <codegen/types.hpp>
#include <hir/type_registry.hpp>
#include <hir/types.hpp>

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

  const hir::FunctionType &as_function(hir::TypeId type_id) const {
    return m_type_registry.as_function(type_id);
  }

  const hir::PrimitiveTypeValue &as_primitive(hir::TypeId type_id) const {
    return m_type_registry.as_primitive(type_id);
  }

  const hir::TypeVariable &as_type_variable(hir::TypeId type_id) const {
    return m_type_registry.as_type_variable(type_id);
  }

  std::string to_string(const auto &type) const {
    return m_type_registry.to_string(type);
  }

  LLVMType to_type(hir::TypeId type_id) const {
    return to_llvm_type(type_registry().get(type_id));
  }

private:
  Module m_module{};
  SymbolTable m_symbol_table{};
  const hir::TypeRegistry &m_type_registry;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
