//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR type system. Concrete types only — no TypeVariable,
//*            no unifier state. Interned in a TypeArena owned by the
//*            ZIR program.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************
#include "hash_combine.hpp"
#include <compare>
#include <cstddef>
#include <type_traits>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

using InnerTypeIdType = std::size_t;

struct TypeId {
  InnerTypeIdType m_id;
  auto operator<=>(const TypeId &) const = default;
};

struct UnitType {
  auto operator<=>(const UnitType &) const = default;
};
struct BoolType {
  auto operator<=>(const BoolType &) const = default;
};
struct CharType {
  auto operator<=>(const CharType &) const = default;
};
struct I8Type {
  auto operator<=>(const I8Type &) const = default;
};
struct I32Type {
  auto operator<=>(const I32Type &) const = default;
};
struct I64Type {
  auto operator<=>(const I64Type &) const = default;
};
struct NeverType {
  auto operator<=>(const NeverType &) const = default;
};

struct FunctionType {
  std::vector<TypeId> m_parameters;
  TypeId m_return_type;
  auto operator<=>(const FunctionType &) const = default;
};

template <typename ZirType> struct ToConcrete : std::false_type {
  using zir_type = ZirType;
};

template <> struct ToConcrete<UnitType> {
  using zir_type = UnitType;
};

template <> struct ToConcrete<BoolType> {
  using zir_type = UnitType;
  using concrete_type = bool;
};

template <> struct ToConcrete<CharType> {
  using zir_type = UnitType;
  using concrete_type = char;
};

template <> struct ToConcrete<I8Type> {
  using zir_type = UnitType;
  using concrete_type = int8_t;
};

template <> struct ToConcrete<I32Type> {
  using zir_type = UnitType;
  using concrete_type = int32_t;
};

template <> struct ToConcrete<I64Type> {
  using zir_type = UnitType;
  using concrete_type = int64_t;
};

using Type = std::variant<UnitType, BoolType, CharType, I8Type, I32Type,
                          I64Type, NeverType, FunctionType>;

//****************************************************************************
} // namespace bust::zir
//****************************************************************************

//****************************************************************************
namespace std {
//****************************************************************************

template <> struct hash<bust::zir::TypeId> {
  size_t operator()(const bust::zir::TypeId &id) const noexcept {
    return hash<bust::zir::InnerTypeIdType>{}(id.m_id);
  }
};

template <> struct hash<bust::zir::FunctionType> {
  size_t operator()(const bust::zir::FunctionType &id) const noexcept {
    size_t seed = 0;
    for (const auto &parameter : id.m_parameters) {
      core::hash_combine(seed, parameter);
    }
    core::hash_combine(seed, id.m_return_type);
    return seed;
  }
};

template <> struct hash<bust::zir::UnitType> {
  size_t operator()(const bust::zir::UnitType &) const noexcept {
    return hash<size_t>{}(0);
  }
};

template <> struct hash<bust::zir::BoolType> {
  size_t operator()(const bust::zir::BoolType &) const noexcept {
    return hash<size_t>{}(1);
  }
};

template <> struct hash<bust::zir::CharType> {
  size_t operator()(const bust::zir::CharType &) const noexcept {
    return hash<size_t>{}(2);
  }
};

template <> struct hash<bust::zir::I8Type> {
  size_t operator()(const bust::zir::I8Type &) const noexcept {
    return hash<size_t>{}(3);
  }
};

template <> struct hash<bust::zir::I32Type> {
  size_t operator()(const bust::zir::I32Type &) const noexcept {
    return hash<size_t>{}(4);
  }
};

template <> struct hash<bust::zir::I64Type> {
  size_t operator()(const bust::zir::I64Type &) const noexcept {
    return hash<size_t>{}(5);
  }
};

template <> struct hash<bust::zir::NeverType> {
  size_t operator()(const bust::zir::NeverType &) const noexcept {
    return hash<size_t>{}(6);
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
