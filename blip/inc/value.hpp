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
#include <memory>
#include <string>
#include <variant>

//****************************************************************************
namespace blip {
//****************************************************************************

// Forward declaration to break dependency
class Environment;

struct Unit {};

struct Function {
  Identifier *m_name;
  std::shared_ptr<Environment> m_environment;
  AstNode *m_body;
};

using Value = std::variant<int, double, bool, std::string, Unit, Function>;

std::string value_to_string(const Value &value);

//****************************************************************************
} // namespace blip
//****************************************************************************
