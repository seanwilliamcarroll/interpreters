//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the ZIR lowering pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>
#include <zir/arena.hpp>
#include <zir/environment.hpp>
#include <zir/nodes.hpp>
#include <zir/type_resolver.hpp>

#include <unordered_map>

#include "zir/types.hpp"

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Context {
  explicit Context(const hir::TypeArena &type_arena,
                   hir::UnifierState unifier_state)
      : m_type_arena(type_arena),
        m_resolver(type_arena, std::move(unifier_state)) {}

  TypeId convert(const hir::TypeKind &type_kind) {
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
              parameters.emplace_back(convert(parameter));
            }
            auto return_type = convert(tk.m_return_type);
            return FunctionType{.m_parameters = std::move(parameters),
                                .m_return_type = return_type};
          } else if constexpr (std::is_same_v<T, hir::NeverType>) {
            return NeverType{};

          } else if constexpr (std::is_same_v<T, hir::TypeVariable>) {
            throw core::InternalCompilerError(
                "Should never happen that ZIR lowerer sees a TypeVariable");
          } else if constexpr (std::is_same_v<T, hir::TupleType>) {
            std::vector<TypeId> fields;
            fields.reserve(tk.m_fields.size());
            for (const auto &field : tk.m_fields) {
              fields.emplace_back(convert(field));
            }
            return TupleType{
                .m_fields = std::move(fields),
            };
          }
        },
        type_kind);

    return m_arena.intern(new_type);
  }

  TypeId convert(hir::TypeId type) {
    auto resolved_type_id = m_resolver.resolve(type);
    return convert(m_type_arena.get(resolved_type_id));
  }

  void set_global_binding(const std::string &name, BindingId id) {
    m_global_bindings[name] = id;
  }

  [[nodiscard]] BindingId get_global_binding(const std::string &name) const {
    auto iter = m_global_bindings.find(name);
    if (iter == m_global_bindings.end()) {
      throw core::InternalCompilerError("Could not find global binding: " +
                                        name);
    }
    return iter->second;
  }

  [[nodiscard]] const std::unordered_map<std::string, BindingId> &
  global_bindings() const {
    return m_global_bindings;
  }
  Environment &env() { return m_env; }
  Arena &arena() { return m_arena; }
  [[nodiscard]] const Arena &arena() const { return m_arena; }

  [[nodiscard]] std::string to_string(hir::TypeId type) const {
    return m_type_arena.to_string(type);
  }

private:
  const hir::TypeArena &m_type_arena;
  const TypeResolver m_resolver;
  Arena m_arena;
  Environment m_env;
  std::unordered_map<std::string, BindingId> m_global_bindings;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
