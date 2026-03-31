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

#include <type.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

std::string type_to_string(Type type) {
  switch (type) {
  case Type::Int:
    return "int";
  case Type::Double:
    return "double";
  case Type::Bool:
    return "bool";
  case Type::String:
    return "string";
  case Type::Unit:
    return "unit";
  case Type::Fn:
    return "fn";
  }
}

Type string_to_type(const std::string &name) {
  if (name == "int") {
    return Type::Int;
  }
  if (name == "double") {
    return Type::Double;
  }
  if (name == "bool") {
    return Type::Bool;
  }
  if (name == "string") {
    return Type::String;
  }
  if (name == "unit") {
    return Type::Unit;
  }
  if (name == "fn") {
    return Type::Fn;
  }
  throw std::runtime_error("Unknown type name: \"" + name + "\"");
}

//****************************************************************************
} // namespace blip
//****************************************************************************
