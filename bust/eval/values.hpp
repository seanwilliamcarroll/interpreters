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

struct Environment {};

template <PrimitiveType InnerType> struct AbstractValue {
  const static PrimitiveType m_type = InnerType;
};

template <> struct AbstractValue<PrimitiveType::BOOL> {
  const static PrimitiveType m_type = PrimitiveType::BOOL;
  bool m_value;
};

template <> struct AbstractValue<PrimitiveType::I64> {
  const static PrimitiveType m_type = PrimitiveType::I64;
  int64_t m_value;
};

using Bool = AbstractValue<PrimitiveType::BOOL>;
using I64 = AbstractValue<PrimitiveType::I64>;
using Unit = AbstractValue<PrimitiveType::UNIT>;

struct Closure {
  std::vector<std::string> m_parameters;
  const hir::Expression *m_expression;
  std::shared_ptr<Environment> m_env;
};

using Value = std::variant<Bool, I64, Unit, Closure>;

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
