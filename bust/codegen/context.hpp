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
#include <zir/arena.hpp>
#include <zir/types.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Context(const zir::Arena &arena) : m_arena(arena) {}

  Module &module() { return m_module; }
  SymbolTable &symbols() { return m_symbol_table; }
  Function &function() { return m_module.current_function(); }
  BasicBlock &block() { return function().current_basic_block(); }

  const zir::Arena &arena() const { return m_arena; }

  std::string to_string(const auto &type) const {
    return m_arena.to_string(type);
  }

  LLVMType to_type(zir::TypeId type_id) const {
    return to_llvm_type(arena().get(type_id));
  }

private:
  Module m_module{};
  SymbolTable m_symbol_table{};
  const zir::Arena &m_arena;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
