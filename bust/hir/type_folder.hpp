//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Generic fold over bust HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>

#include <variant>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

template <typename TypeVariableFunctor>
TypeId fold(TypeArena &type_arena, TypeId type_id,
            TypeVariableFunctor &&functor) {
  auto type_kind = type_arena.get(type_id);
  return std::visit(
      [&](auto &&tk) -> TypeId {
        using T = std::decay_t<decltype(tk)>;
        if constexpr (std::is_same_v<T, PrimitiveTypeValue>) {
          return type_arena.intern(tk);
        } else if constexpr (std::is_same_v<T, TypeVariable>) {
          return functor(tk);
        } else if constexpr (std::is_same_v<T, FunctionType>) {
          std::vector<TypeId> parameters;
          parameters.reserve(tk.m_parameters.size());
          for (const auto &parameter : tk.m_parameters) {
            parameters.emplace_back(fold(type_arena, parameter, functor));
          }
          return type_arena.intern(FunctionType{
              .m_parameters = std::move(parameters),
              .m_return_type = fold(type_arena, tk.m_return_type, functor),
          });
        } else if constexpr (std::is_same_v<T, TupleType>) {
          std::vector<TypeId> fields;
          fields.reserve(tk.m_fields.size());
          for (const auto &field : tk.m_fields) {
            fields.emplace_back(fold(type_arena, field, functor));
          }
          return type_arena.intern(TupleType{
              .m_fields = std::move(fields),
          });
        } else if constexpr (std::is_same_v<T, NeverType>) {
          return type_arena.m_never;
        }
      },
      type_kind);
}

template <typename TypeVariableFunctor>
void walk(TypeArena &type_arena, TypeId type_id,
          TypeVariableFunctor &&functor) {
  auto type_kind = type_arena.get(type_id);
  std::visit(
      [&](auto &&tk) {
        using T = std::decay_t<decltype(tk)>;
        if constexpr (std::is_same_v<T, TypeVariable>) {
          functor(tk);
        } else if constexpr (std::is_same_v<T, FunctionType>) {
          for (const auto &parameter : tk.m_parameters) {
            walk(type_arena, parameter, functor);
          }
          walk(type_arena, tk.m_return_type, functor);
        } else if constexpr (std::is_same_v<T, TupleType>) {
          for (const auto &field : tk.m_fields) {
            walk(type_arena, field, functor);
          }
        } else if constexpr (std::is_same_v<T, PrimitiveTypeValue> ||
                             std::is_same_v<T, NeverType>) {
          // Do nothing
        } else {
          static_assert(false, "Unhandled type!");
        }
      },
      type_kind);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
