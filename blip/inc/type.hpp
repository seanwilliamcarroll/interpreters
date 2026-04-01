//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type representation for the blip type checker
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <string>
#include <variant>
#include <vector>

//****************************************************************************
namespace blip {
//****************************************************************************

struct FunctionType;

enum class BaseType : uint8_t {
  Int,
  Double,
  Bool,
  String,
  Unit,
};

using Type = std::variant<BaseType, std::shared_ptr<FunctionType>>;

struct FunctionType {
  std::vector<Type> m_parameter_types;
  Type m_return_type;
};

bool operator==(const Type &, const Type &);
bool operator==(const FunctionType &, const FunctionType &);

std::string type_to_string(const Type &type);
std::string type_to_string(BaseType type);
std::string type_to_string(const FunctionType &type);

Type string_to_type(const std::string &name);

//****************************************************************************
} // namespace blip
//****************************************************************************
