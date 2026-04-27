//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type definitions for bust HIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hash_combine.hpp>
#include <types.hpp>

#include <functional>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct HirNamespace {};
using TypeId = bust::TypeId<HirNamespace>;

struct PrimitiveTypeValue {
  PrimitiveType m_type;
  bool operator==(const PrimitiveTypeValue &) const = default;
};

struct TypeVariable {
  size_t m_id;
  bool operator==(const TypeVariable &) const = default;
};

struct FunctionType {
  std::vector<TypeId> m_parameters;
  TypeId m_return_type;
  bool operator==(const FunctionType &) const = default;
};

struct TupleType {
  std::vector<TypeId> m_fields;
  bool operator==(const TupleType &) const = default;
};

struct NeverType {
  bool operator==(const NeverType &) const = default;
};

using TypeKind = std::variant<PrimitiveTypeValue, TypeVariable, FunctionType,
                              TupleType, NeverType>;

inline bool is_type_in_type_class(PrimitiveTypeClass type_class,
                                  const TypeKind &type) {
  return std::visit(
      [&](const auto &tk) -> bool {
        using T = std::decay_t<decltype(tk)>;
        if constexpr (std::is_same_v<T, PrimitiveTypeValue>) {
          return bust::is_type_in_type_class(type_class, tk.m_type);
        } else {
          return false;
        }
      },
      type);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************

namespace std {

template <> struct hash<bust::hir::PrimitiveTypeValue> {
  size_t operator()(const bust::hir::PrimitiveTypeValue &id) const noexcept {
    using InnerType = std::underlying_type_t<bust::PrimitiveType>;
    return hash<InnerType>{}(static_cast<InnerType>(id.m_type));
  }
};

template <> struct hash<bust::hir::TypeVariable> {
  size_t operator()(const bust::hir::TypeVariable &id) const noexcept {
    return hash<size_t>{}(id.m_id);
  }
};

template <> struct hash<bust::hir::FunctionType> {
  size_t operator()(const bust::hir::FunctionType &id) const noexcept {
    size_t seed = 0;
    for (const auto &parameter : id.m_parameters) {
      core::hash_combine(seed, parameter);
    }
    core::hash_combine(seed, id.m_return_type);
    return seed;
  }
};

template <> struct hash<bust::hir::TupleType> {
  size_t operator()(const bust::hir::TupleType &id) const noexcept {
    size_t seed = 0;
    for (const auto &field : id.m_fields) {
      core::hash_combine(seed, field);
    }
    return seed;
  }
};

template <> struct hash<bust::hir::NeverType> {
  size_t operator()(const bust::hir::NeverType & /*unused*/) const noexcept {
    return hash<size_t>{}(0);
  }
};

} // namespace std
