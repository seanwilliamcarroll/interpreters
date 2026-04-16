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

  TypeId convert(hir::TypeId type_id, const hir::TypeRegistry &reg) {
    return convert(reg.get(type_id), reg);
  }

  TypeId convert(const hir::TypeKind &type_kind, const hir::TypeRegistry &reg) {
    // Translate concrete hir TypeKind into zir Type and intern
    auto new_type = std::visit(
        [&](const auto &tk) -> Type {
          using T = std::decay_t<decltype(tk)>;
          if constexpr (std::is_same_v<T, hir::PrimitiveTypeValue>) {
            switch (tk.m_type) {
            case PrimitiveType::UNIT:
              return UnitType{};
            case PrimitiveType::BOOL:
              return BoolType{};
            case PrimitiveType::CHAR:
              return CharType{};
            case PrimitiveType::I8:
              return I8Type{};
            case PrimitiveType::I32:
              return I32Type{};
            case PrimitiveType::I64:
              return I64Type{};
            }
          } else if constexpr (std::is_same_v<T, hir::FunctionType>) {
            std::vector<TypeId> parameters;
            parameters.reserve(tk.m_parameters.size());
            for (const auto &parameter : tk.m_parameters) {
              parameters.emplace_back(convert(parameter, reg));
            }
            auto return_type = convert(tk.m_return_type, reg);
            return FunctionType{.m_parameters = std::move(parameters),
                                .m_return_type = return_type};
          } else if constexpr (std::is_same_v<T, hir::NeverType>) {
            return NeverType{};
          } else {
            // TypeVariable shouldn't be passed here?
            std::unreachable();
          }
        },
        type_kind);

    return intern(new_type);
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

private:
  TypeArena m_type_arena{};
  AbstractArena<Expression, ExprId> m_expr_arena{};
  AbstractArena<Binding, BindingId> m_binding_arena{};
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
