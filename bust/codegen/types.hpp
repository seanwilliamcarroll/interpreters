//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : LLVM type and operator enums for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <array>
#include <cassert>
#include <cstdint>
#include <ostream>
#include <utility>
#include <variant>
#include <zir/types.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

enum class LLVMType : uint8_t {
  I1,
  I8,
  I32,
  I64,
  PTR,
  VOID,
};

inline std::ostream &operator<<(std::ostream &out, LLVMType type) {
  constexpr static std::array type_names{"i1",  "i8",  "i32",
                                         "i64", "ptr", "void"};
  return out << type_names[std::to_underlying(type)];
}

inline size_t width_bits(LLVMType type) {
  switch (type) {
  case LLVMType::I1:
    return 1;
  case LLVMType::I8:
    return 8;
  case LLVMType::I32:
    return 32;
  case LLVMType::I64:
  case LLVMType::PTR:
    return 64;
  case LLVMType::VOID:
    std::unreachable();
  }
}

inline LLVMType to_llvm_type(const zir::Type &type) {
  return std::visit(
      [](const auto &t) {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, zir::UnitType>) {
          return LLVMType::VOID;
        } else if constexpr (std::is_same_v<T, zir::BoolType>) {
          return LLVMType::I1;
        } else if constexpr (std::is_same_v<T, zir::I8Type> ||
                             std::is_same_v<T, zir::CharType>) {
          return LLVMType::I8;
        } else if constexpr (std::is_same_v<T, zir::I32Type>) {
          return LLVMType::I32;
        } else if constexpr (std::is_same_v<T, zir::I64Type>) {
          return LLVMType::I64;
        } else if constexpr (std::is_same_v<T, zir::FunctionType>) {
          return LLVMType::PTR;
        } else {
          assert(false && "codegen only handles primitive types and function "
                          "types for now");
          return LLVMType::VOID;
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
