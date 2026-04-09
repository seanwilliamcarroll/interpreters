//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LLVM type and operator enums for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/types.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <ostream>
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

enum class LLVMType : uint8_t {
  I1,
  I8,
  I32,
  I64,
  VOID,
};

inline std::ostream &operator<<(std::ostream &out, LLVMType type) {
  constexpr static std::array type_names{"i1", "i8", "i32", "i64", "void"};
  return out << type_names[std::to_underlying(type)];
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

inline LLVMType to_llvm_type(const hir::Type &type) {
  const auto *prim = std::get_if<hir::PrimitiveTypeValue>(&type);
  assert(prim && "codegen only handles primitive types for now");
  switch (prim->m_type) {
  case PrimitiveType::BOOL:
    return LLVMType::I1;
  case PrimitiveType::I8:
  case PrimitiveType::CHAR:
    return LLVMType::I8;
  case PrimitiveType::I32:
    return LLVMType::I32;
  case PrimitiveType::I64:
    return LLVMType::I64;
  case PrimitiveType::UNIT:
    return LLVMType::VOID;
  }
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
