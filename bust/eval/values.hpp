//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Value types for bust tree-walking evaluator.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/nodes.hpp"
#include "types.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct Scope;

template <PrimitiveType InnerType> struct AbstractValue {
  constexpr static PrimitiveType m_type = InnerType;
};

template <> struct AbstractValue<PrimitiveType::BOOL> {
  constexpr static PrimitiveType m_type = PrimitiveType::BOOL;
  bool m_value;
};

template <> struct AbstractValue<PrimitiveType::CHAR> {
  constexpr static PrimitiveType m_type = PrimitiveType::CHAR;
  char m_value;
};

template <> struct AbstractValue<PrimitiveType::I8> {
  constexpr static PrimitiveType m_type = PrimitiveType::I8;
  int8_t m_value;
};

template <> struct AbstractValue<PrimitiveType::I32> {
  constexpr static PrimitiveType m_type = PrimitiveType::I32;
  int32_t m_value;
};

template <> struct AbstractValue<PrimitiveType::I64> {
  constexpr static PrimitiveType m_type = PrimitiveType::I64;
  int64_t m_value;
};

using Bool = AbstractValue<PrimitiveType::BOOL>;
using Char = AbstractValue<PrimitiveType::CHAR>;
using I8 = AbstractValue<PrimitiveType::I8>;
using I32 = AbstractValue<PrimitiveType::I32>;
using I64 = AbstractValue<PrimitiveType::I64>;
using Unit = AbstractValue<PrimitiveType::UNIT>;

struct Closure {
  std::vector<std::string> m_parameters;
  const hir::Block *m_expression;
  std::shared_ptr<Scope> m_scope;
};

using Value = std::variant<Bool, Char, I8, I32, I64, Unit, Closure>;

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
