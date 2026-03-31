//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Runtime value type for blip evaluation
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "ast.hpp"
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

//****************************************************************************
namespace blip {
//****************************************************************************

// Forward declaration to break dependency
class ValueEnvironment;

struct Unit {};

struct Function {
  std::string m_name;
  std::vector<const Identifier *> m_arguments;
  std::shared_ptr<ValueEnvironment> m_environment;
  const AstNode *m_body;
};

struct BuiltInFunction;

using Value = std::variant<int, double, bool, std::string, Unit, Function,
                           BuiltInFunction>;

struct BuiltInFunction {
  std::string m_name;
  size_t m_expected_arguments;
  std::function<Value(std::vector<Value>)> m_native_function;
};

std::string value_to_string(const Value &value);

//****************************************************************************
} // namespace blip
//****************************************************************************
