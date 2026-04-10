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

#include <cstdint>
#include <hir/nodes.hpp>
#include <memory>
#include <string>
#include <types.hpp>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct Scope;

template <PrimitiveType InnerType> struct AbstractValue {
  constexpr static PrimitiveType m_type = InnerType;
  ToConcrete<m_type>::type m_value;
};

template <> struct AbstractValue<PrimitiveType::UNIT> {};

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
