//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : LLVM type and operator enums for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>
#include <zir/types.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <ostream>
#include <type_traits>
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct CodegenNamespace {};
using TypeId = bust::TypeId<CodegenNamespace>;

struct VoidType {
  auto operator<=>(const VoidType &) const = default;
};
struct I1Type {
  auto operator<=>(const I1Type &) const = default;
  static constexpr size_t width_bits = 1;
};
struct I8Type {
  auto operator<=>(const I8Type &) const = default;
  static constexpr size_t width_bits = 8;
};
struct I32Type {
  auto operator<=>(const I32Type &) const = default;
  static constexpr size_t width_bits = 32;
};
struct I64Type {
  auto operator<=>(const I64Type &) const = default;
  static constexpr size_t width_bits = 64;
};
struct PtrType {
  auto operator<=>(const PtrType &) const = default;
  static constexpr size_t width_bits = 64;
};
struct StructType {
  auto operator<=>(const StructType &) const = default;
  std::vector<TypeId> m_fields;
};
// TODO
// struct ArrayType {  auto operator<=>(const ArrayType &) const = default;
// };

using LLVMType = std::variant<VoidType, I1Type, I8Type, I32Type, I64Type,
                              PtrType, StructType>;

constexpr size_t BITS_PER_BYTE = 8;

inline size_t width_bits(LLVMType type) {
  return std::visit(
      [](const auto &t) -> size_t {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, I1Type> || std::is_same_v<T, I8Type> ||
                      std::is_same_v<T, I32Type> ||
                      std::is_same_v<T, I64Type> ||
                      std::is_same_v<T, PtrType>) {
          return std::remove_reference_t<decltype(t)>::width_bits;
        } else {
          throw core::InternalCompilerError("Bad type to call width_bytes on");
        }
      },
      type);
}

enum class LLVMBinaryOperator : uint8_t {
  ADD,
  SUB,
  MUL,
  SDIV,
  SREM,
};

inline std::ostream &operator<<(std::ostream &out, LLVMBinaryOperator op) {
  constexpr static std::array op_names{"add", "sub", "mul", "sdiv", "srem"};
  return out << op_names[std::to_underlying(op)];
}

enum class LLVMIntegerCompareCondition : uint8_t {
  EQ,
  NE,
  UGT,
  UGE,
  ULT,
  ULE,
  SGT,
  SGE,
  SLT,
  SLE,
};

inline std::ostream &operator<<(std::ostream &out,
                                LLVMIntegerCompareCondition op) {
  constexpr static std::array op_names{
      "eq", "ne", "ugt", "uge", "ult", "ule", "sgt", "sge", "slt", "sle",
  };
  return out << op_names[std::to_underlying(op)];
}

enum class LLVMCastOperator : uint8_t {
  SEXT,
  ZEXT,
  TRUNC,
};

inline std::ostream &operator<<(std::ostream &out, LLVMCastOperator op) {
  constexpr static std::array op_names{"sext", "zext", "trunc"};
  return out << op_names[std::to_underlying(op)];
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************

//****************************************************************************
namespace std {
//****************************************************************************

template <> struct hash<bust::codegen::VoidType> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::codegen::VoidType & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::codegen::I1Type> {
  static constexpr size_t HASH_VALUE = 1;
  size_t operator()(const bust::codegen::I1Type & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::codegen::I8Type> {
  static constexpr size_t HASH_VALUE = 2;
  size_t operator()(const bust::codegen::I8Type & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::codegen::I32Type> {
  static constexpr size_t HASH_VALUE = 3;
  size_t operator()(const bust::codegen::I32Type & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::codegen::I64Type> {
  static constexpr size_t HASH_VALUE = 4;
  size_t operator()(const bust::codegen::I64Type & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::codegen::PtrType> {
  static constexpr size_t HASH_VALUE = 5;
  size_t operator()(const bust::codegen::PtrType & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::codegen::StructType> {
  size_t operator()(const bust::codegen::StructType &id) const noexcept {
    size_t seed = 0;
    for (const auto &field : id.m_fields) {
      core::hash_combine(seed, field);
    }
    return seed;
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
