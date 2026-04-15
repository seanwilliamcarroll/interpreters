//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR arena — interning storage for ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

//****************************************************************************
#include "exceptions.hpp"
#include "zir/nodes.hpp"
#include "zir/types.hpp"
#include <unordered_map>
namespace bust::zir {
//****************************************************************************

struct TypeArena {
  explicit TypeArena()
      : m_unit(intern(UnitType{})), m_bool(intern(BoolType{})),
        m_char(intern(CharType{})), m_i8(intern(I8Type{})),
        m_i32(intern(I32Type{})), m_i64(intern(I64Type{})),
        m_never(intern(NeverType{})) {}

  TypeId intern(const Type &type) {
    auto iter = m_type_to_type_id.find(type);
    if (iter == m_type_to_type_id.end()) {
      auto new_id = m_type_id_to_type.size();
      m_type_to_type_id.emplace(type, new_id);
      m_type_id_to_type.emplace_back(type);
      return {new_id};
    }
    return iter->second;
  }

  const Type &get(TypeId type_id) const {
    if (type_id.m_id >= m_type_id_to_type.size()) {
      throw core::InternalCompilerError("Failed to call get on type_id: " +
                                        std::to_string(type_id.m_id));
    }
    return m_type_id_to_type[type_id.m_id];
  }

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
  IdType push(const ActualType &actual) {
    auto new_id = m_id_to_actual.size();
    m_actual_to_id.emplace(actual, new_id);
    m_id_to_actual.emplace_back(actual);
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
private:
  TypeArena m_type_arena{};
  AbstractArena<Expression, ExprId> m_expr_arena{};
  AbstractArena<Binding, BindingId> m_binding_arena{};
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
