//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR arena — interning storage for ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <arena.hpp>
#include <exceptions.hpp>
#include <types.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

#include <string>
#include <utility>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct TypeArena : public AbstractInternArena<TypeId, Type> {
  explicit TypeArena() = default;

  [[nodiscard]] const FunctionType &as_function(TypeId type_id) const {
    return as<FunctionType>(type_id);
  }

  [[nodiscard]] std::string to_string(TypeId type_id) const override;
  [[nodiscard]] std::string to_string(const Type &type) const override;
};

struct ExpressionArena : public AbstractPushArena<ExprId, Expression> {
  // TODO
  [[nodiscard]] std::string to_string(ExprId /*unused*/) const override {
    return {};
  }
  [[nodiscard]] std::string
  to_string(const Expression & /*unused*/) const override {
    return {};
  }
};

struct BindingArena : public AbstractPushArena<BindingId, Binding> {
  // TODO
  [[nodiscard]] std::string to_string(BindingId /*unused*/) const override {
    return {};
  }
  [[nodiscard]] std::string
  to_string(const Binding & /*unused*/) const override {
    return {};
  }
};

struct Arena {
  explicit Arena()
      : m_unit(type().intern(UnitType{})), m_bool(type().intern(BoolType{})),
        m_char(type().intern(CharType{})), m_i8(type().intern(I8Type{})),
        m_i32(type().intern(I32Type{})), m_i64(type().intern(I64Type{})),
        m_never(type().intern(NeverType{})) {}

  TypeArena &type() { return m_type_arena; }
  [[nodiscard]] const TypeArena &type() const { return m_type_arena; }

  ExpressionArena &expr() { return m_expr_arena; }
  [[nodiscard]] const ExpressionArena &expr() const { return m_expr_arena; }

  BindingArena &binding() { return m_binding_arena; }
  [[nodiscard]] const BindingArena &binding() const { return m_binding_arena; }

  [[nodiscard]] const Type &get(TypeId id) const { return type().get(id); }
  [[nodiscard]] const Expression &get(ExprId id) const {
    return expr().get(id);
  }
  [[nodiscard]] const Binding &get(BindingId id) const {
    return binding().get(id);
  }

  ExprId push(Expression actual) { return expr().push(std::move(actual)); }
  BindingId push(Binding actual) { return binding().push(std::move(actual)); }

  TypeId intern(const Type &input_type) { return type().intern(input_type); }

  template <typename VariantType> const VariantType &as(TypeId id) const {
    return type().as<VariantType>(id);
  }

  [[nodiscard]] const FunctionType &as_function(TypeId type_id) const {
    return type().as_function(type_id);
  }

  [[nodiscard]] std::string to_string(const Type &type) const {
    return m_type_arena.to_string(type);
  }

  [[nodiscard]] std::string to_string(TypeId type_id) const {
    return m_type_arena.to_string(type_id);
  }

  [[nodiscard]] TypeId get_block_type(const Block &block) const {
    if (block.m_final_expression.has_value()) {
      const auto &expression = get(block.m_final_expression.value());
      return expression.m_type_id;
    }
    return m_unit;
  }

private:
  TypeArena m_type_arena;
  ExpressionArena m_expr_arena{};
  BindingArena m_binding_arena{};

public:
  TypeId m_unit;
  TypeId m_bool;
  TypeId m_char;
  TypeId m_i8;
  TypeId m_i32;
  TypeId m_i64;
  TypeId m_never;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
