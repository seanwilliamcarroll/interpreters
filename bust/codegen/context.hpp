//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/arena.hpp>
#include <codegen/basic_block.hpp>
#include <codegen/function.hpp>
#include <codegen/module.hpp>
#include <codegen/naming_conventions.hpp>
#include <codegen/parameter.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <zir/arena.hpp>
#include <zir/types.hpp>

#include <cassert>
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Context(const zir::Arena &arena)
      : m_arena(arena), m_void(m_type_arena.intern(VoidType{})),
        m_i1(m_type_arena.intern(I1Type{})),
        m_i8(m_type_arena.intern(I8Type{})),
        m_i32(m_type_arena.intern(I32Type{})),
        m_i64(m_type_arena.intern(I64Type{})),
        m_ptr(m_type_arena.intern(PtrType{})),
        m_fat_ptr(m_type_arena.intern_global_struct(
            std::string{conventions::closure_type},
            StructType{.m_fields = {m_ptr, m_ptr}})) {}

  Module &module() { return m_module; }
  SymbolTable &symbols() { return m_symbol_table; }
  Function &function() { return m_module.current_function(); }
  BasicBlock &block() { return function().current_basic_block(); }

  [[nodiscard]] const zir::Arena &arena() const { return m_arena; }

  [[nodiscard]] std::string to_string(const zir::Type &type) const {
    return m_arena.to_string(type);
  }

  [[nodiscard]] std::string to_string(TypeId type) const {
    return m_type_arena.to_string(type);
  }

  [[nodiscard]] std::string to_string(const LLVMType &type) const {
    return m_type_arena.to_string(type);
  }

  [[nodiscard]] TypeId to_type(zir::TypeId type_id) const {
    return m_type_arena.intern(to_llvm_type(arena().get(type_id)));
  }

  TypeArena &type() { return m_type_arena; }

  [[nodiscard]]
  const TypeArena &type() const {
    return m_type_arena;
  }

  [[nodiscard]]
  Parameter env_parameter() const {
    return {
        .m_name = ParameterHandle{std::string{conventions::env_parameter_name}},
        .m_type = m_ptr,
    };
  }

private:
  Module m_module;
  SymbolTable m_symbol_table;
  const zir::Arena &m_arena;
  TypeArena m_type_arena;

public:
  TypeId m_void;
  TypeId m_i1;
  TypeId m_i8;
  TypeId m_i32;
  TypeId m_i64;
  TypeId m_ptr;
  TypeId m_fat_ptr;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
