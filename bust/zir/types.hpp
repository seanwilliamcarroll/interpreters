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
#include <hash_combine.hpp>
#include <types.hpp>

#include <cstddef>
#include <type_traits>
#include <variant>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct ZirNamespace {};
using TypeId = bust::TypeId<ZirNamespace>;

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
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::UnitType & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::BoolType> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::BoolType & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::CharType> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::CharType & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::I8Type> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::I8Type & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::I32Type> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::I32Type & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::I64Type> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::I64Type & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::NeverType> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::NeverType & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
