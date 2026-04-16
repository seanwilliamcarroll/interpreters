//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR arena — interning storage for ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************
#include "exceptions.hpp"
#include "hir/type_registry.hpp"
#include "hir/types.hpp"
#include "types.hpp"
#include "zir/nodes.hpp"
#include "zir/types.hpp"
#include <string>
#include <unordered_map>
#include <utility>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct TypeArena {
  explicit TypeArena()
      : m_unit(intern(UnitType{})), m_bool(intern(BoolType{})),
        m_char(intern(CharType{})), m_i8(intern(I8Type{})),
        m_i32(intern(I32Type{})), m_i64(intern(I64Type{})),
        m_never(intern(NeverType{})) {}

  TypeId intern(const Type &);
  TypeId intern(const Type &) const;

  const Type &get(TypeId) const;

  TypeId convert(hir::TypeId, const hir::TypeRegistry &);

  TypeId convert(const hir::TypeKind &, const hir::TypeRegistry &);

  template <typename VariantType>
  const VariantType &as(TypeId type_id, const char *function) const {
    const auto &type_kind = get(type_id);
    if (!std::holds_alternative<VariantType>(type_kind)) {
      throw core::InternalCompilerError(std::string(function) +
                                        " Bad access to registry with " +
                                        to_string(type_id));
    }
    return std::get<VariantType>(type_kind);
  }

  const FunctionType &as_function(TypeId type_id) const {
    return as<FunctionType>(type_id, __PRETTY_FUNCTION__);
  }

  template <typename VariantType> bool is(TypeId type_id) const {
    const auto &type_kind = get(type_id);
    return std::holds_alternative<VariantType>(type_kind);
  }

  bool is_function(TypeId type_id) const { return is<FunctionType>(type_id); }

  std::string to_string(TypeId) const;
  std::string to_string(const Type &) const;

private:
  std::unordered_map<Type, TypeId> m_type_to_type_id{};
  std::vector<Type> m_type_id_to_type{};

public:
  TypeId m_unit;
  TypeId m_bool;
  TypeId m_char;
  TypeId m_i8;
  TypeId m_i32;
  TypeId m_i64;
  TypeId m_never;
};

template <typename ActualType, typename IdType> struct AbstractArena {
  IdType push(ActualType actual) {
    auto new_id = m_id_to_actual.size();
    m_actual_to_id.emplace(actual, new_id);
    m_id_to_actual.emplace_back(std::move(actual));
    return {new_id};
  }

  const ActualType &get(IdType type_id) const {
    if (type_id.m_id >= m_id_to_actual.size()) {
      throw core::InternalCompilerError("Failed to call get on type_id: " +
                                        std::to_string(type_id.m_id));
    }
    return m_id_to_actual[type_id.m_id];
  }

private:
  std::unordered_map<ActualType, IdType> m_actual_to_id{};
  std::vector<ActualType> m_id_to_actual{};
};

struct Arena {
  TypeArena &type() { return m_type_arena; }
  const TypeArena &type() const { return m_type_arena; }

  AbstractArena<Expression, ExprId> &expr() { return m_expr_arena; }
  const AbstractArena<Expression, ExprId> &expr() const { return m_expr_arena; }

  AbstractArena<Binding, BindingId> &binding() { return m_binding_arena; }
  const AbstractArena<Binding, BindingId> &binding() const {
    return m_binding_arena;
  }

  const Type &get(TypeId id) const { return type().get(id); }
  const Expression &get(ExprId id) const { return expr().get(id); }
  const Binding &get(BindingId id) const { return binding().get(id); }

  ExprId push(Expression actual) { return expr().push(std::move(actual)); }
  BindingId push(Binding actual) { return binding().push(std::move(actual)); }

  TypeId intern(const Type &input_type) { return type().intern(input_type); }

  TypeId convert(const hir::TypeKind &type_kind, const hir::TypeRegistry &reg) {
    return type().convert(type_kind, reg);
  }
  TypeId convert(hir::TypeId type_id, const hir::TypeRegistry &reg) {
    return type().convert(type_id, reg);
  }

  const FunctionType &as_function(TypeId type_id) const {
    return type().as_function(type_id);
  }

  std::string to_string(const Type &type) const {
    return m_type_arena.to_string(type);
  }

  std::string to_string(TypeId type_id) const {
    return m_type_arena.to_string(type_id);
  }

private:
  TypeArena m_type_arena{};
  AbstractArena<Expression, ExprId> m_expr_arena{};
  AbstractArena<Binding, BindingId> m_binding_arena{};
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
