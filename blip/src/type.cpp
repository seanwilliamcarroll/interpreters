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

#include "exceptions.hpp"
#include <memory>
#include <sstream>
#include <stdexcept>
#include <type.hpp>
#include <type_traits>
#include <variant>

//****************************************************************************
namespace blip {
//****************************************************************************

bool operator==(const Type &type_a, const Type &type_b) {
  if (type_a.index() != type_b.index()) {
    return false;
  }

  // They must be the same type

  bool ret = false;
  std::visit(
      [&](auto &&arg) {
        using UnderlyingType = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<UnderlyingType, BaseType>) {
          ret = std::get<BaseType>(type_a) == std::get<BaseType>(type_b);
        } else if constexpr (std::is_same_v<UnderlyingType,
                                            std::shared_ptr<FunctionType>>) {
          ret = *std::get<std::shared_ptr<FunctionType>>(type_a) ==
                *std::get<std::shared_ptr<FunctionType>>(type_b);
        }
      },
      type_a);
  return ret;
}

bool operator==(const FunctionType &type_a, const FunctionType &type_b) {
  if (type_a.m_parameter_types.size() != type_b.m_parameter_types.size()) {
    return false;
  }
  for (size_t index = 0; index < type_a.m_parameter_types.size(); ++index) {
    if (type_a.m_parameter_types[index] != type_b.m_parameter_types[index]) {
      return false;
    }
  }
  return type_a.m_return_type == type_b.m_return_type;
}

std::string type_to_string(BaseType type) {
  switch (type) {
  case BaseType::Int:
    return "int";
  case BaseType::Double:
    return "double";
  case BaseType::Bool:
    return "bool";
  case BaseType::String:
    return "string";
  case BaseType::Unit:
    return "unit";
  }
}

std::string type_to_string(const FunctionType &type) {
  std::stringstream output;
  output << "(";
  for (size_t i = 0; i < type.m_parameter_types.size(); ++i) {
    if (i > 0) {
      output << ", ";
    }
    output << type_to_string(type.m_parameter_types[i]);
  }
  output << ") -> " << type_to_string(type.m_return_type);
  return output.str();
}

std::string type_to_string(const Type &type) {
  return std::visit(
      [](auto &&arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, BaseType>) {
          return type_to_string(arg);
        } else {
          return type_to_string(*arg);
        }
      },
      type);
}

Type string_to_type(const std::string &name) {
  if (name == "int") {
    return BaseType::Int;
  }
  if (name == "double") {
    return BaseType::Double;
  }
  if (name == "bool") {
    return BaseType::Bool;
  }
  if (name == "string") {
    return BaseType::String;
  }
  if (name == "unit") {
    return BaseType::Unit;
  }
  throw std::runtime_error("Unknown type name: \"" + name + "\"");
}

//****************************************************************************
} // namespace blip
//****************************************************************************
