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
#include <codegen/ir_builder.hpp>
#include <codegen/module.hpp>
#include <codegen/naming_conventions.hpp>
#include <codegen/parameter.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <zir/arena.hpp>
#include <zir/types.hpp>

#include <cassert>
#include <string>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Context {
  Context(const zir::Arena &arena)
      : m_arena(arena), m_builder(*this),
        m_void(m_type_arena.intern(VoidType{})),
        m_i1(m_type_arena.intern(I1Type{})),
        m_i8(m_type_arena.intern(I8Type{})),
        m_i32(m_type_arena.intern(I32Type{})),
        m_i64(m_type_arena.intern(I64Type{})),
        m_ptr(m_type_arena.intern(PtrType{})),
        m_fat_ptr(m_type_arena.intern(StructType{
            .m_fields = {m_ptr, m_ptr},
            .m_name =
                std::make_optional(std::string{conventions::closure_type}),
        })),
        m_allocator_symbol({
            .m_handle =
                GlobalHandle{
                    .m_handle = std::string{conventions::allocator_function},
                },
            .m_type_id = m_ptr,
        }) {}

  Module &module() { return m_module; }
  SymbolTable &symbols() { return m_symbol_table; }

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

  [[nodiscard]] TypeId to_type(zir::TypeId type_id) {
    return m_type_arena.intern(to_type(arena().get(type_id)));
  }

  [[nodiscard]] LLVMType to_type(const zir::Type &type) {
    return std::visit(
        [&](const auto &t) -> LLVMType {
          using T = std::decay_t<decltype(t)>;
          if constexpr (std::is_same_v<T, zir::UnitType>) {
            return VoidType{};
          } else if constexpr (std::is_same_v<T, zir::BoolType>) {
            return I1Type{};
          } else if constexpr (std::is_same_v<T, zir::I8Type> ||
                               std::is_same_v<T, zir::CharType>) {
            return I8Type{};
          } else if constexpr (std::is_same_v<T, zir::I32Type>) {
            return I32Type{};
          } else if constexpr (std::is_same_v<T, zir::I64Type>) {
            return I64Type{};
          } else if constexpr (std::is_same_v<T, zir::FunctionType>) {
            return PtrType{};
          } else if constexpr (std::is_same_v<T, zir::TupleType>) {
            std::vector<TypeId> fields;
            fields.reserve(t.m_fields.size());
            for (const auto &field : t.m_fields) {
              fields.emplace_back(this->to_type(field));
            }
            return StructType{
                .m_fields = std::move(fields),
                .m_name{},
            };
          } else {
            assert(false && "codegen only handles primitive types and function "
                            "types for now");
            return VoidType{};
          }
        },
        type);
  }

  TypeArena &type() { return m_type_arena; }

  [[nodiscard]]
  const TypeArena &type() const {
    return m_type_arena;
  }

  [[nodiscard]]
  Value env() const {
    return {
        .m_handle = NamedHandle{std::string{conventions::env_parameter_name}},
        .m_type_id = m_ptr,
    };
  }

  IRBuilder &builder() { return m_builder; }

  [[nodiscard]] const Value &allocator_symbol() const {
    return m_allocator_symbol;
  }

  AllocaBinding define_local(const std::string &name, TypeId inner_type_id,
                             Value initial_value) {
    auto alloca_slot = builder().emit_alloca(
        inner_type_id, uniqify_name(conventions::make_alloca_name(name)));
    builder().emit_store(alloca_slot, std::move(initial_value));
    auto binding = AllocaBinding{
        .m_ptr = alloca_slot,
        .m_internal_type_id = inner_type_id,
    };
    symbols().bind_local(name, binding);
    return binding;
  }

  ClosureBinding define_local_closure(const std::string &name,
                                      Value initial_value) {
    auto alloca_slot = builder().emit_alloca(
        m_ptr, uniqify_name(conventions::make_alloca_name(name)));
    builder().emit_store(alloca_slot, std::move(initial_value));
    auto binding = ClosureBinding{
        .m_ptr = alloca_slot,
    };
    symbols().bind_local(name, binding);
    return binding;
  }

  FunctionBinding define_function(const std::string &name, Value callee,
                                  TypeId return_type_id,
                                  std::vector<TypeId> parameter_types) {
    // Probably should emit a define or something?
    auto binding = FunctionBinding{
        .m_callee = std::move(callee),
        .m_return_type = return_type_id,
        .m_parameter_types = std::move(parameter_types),
    };
    symbols().bind_global(name, binding);
    return binding;
  }

  void emit_parameter_prologue(const std::vector<Parameter> &parameters) {
    // Make allocas for all parameters
    for (const auto &parameter : parameters) {
      define_local(parameter.m_name, parameter.m_type,
                   {
                       .m_handle =
                           NamedHandle{
                               parameter.m_name,
                           },
                       .m_type_id = parameter.m_type,
                   });
    }
  }

  std::string uniqify_name(const std::string &name) {
    return m_name_tracker.uniquify(name);
  }

private:
  Module m_module;
  SymbolTable m_symbol_table;
  const zir::Arena &m_arena;
  TypeArena m_type_arena;
  IRBuilder m_builder;

public:
  TypeId m_void;
  TypeId m_i1;
  TypeId m_i8;
  TypeId m_i32;
  TypeId m_i64;
  TypeId m_ptr;
  TypeId m_fat_ptr;

private:
  Value m_allocator_symbol;
  UniqueNameTracker m_name_tracker{};
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
