//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Environment implementation for blip evaluation
//*
//*
//****************************************************************************

#include "value.hpp"
#include <environment.hpp>
#include <exceptions.hpp>
#include <memory>
#include <stdexcept>
#include <variant>

//****************************************************************************
namespace blip {
namespace {
//****************************************************************************

double as_number(const Value &v) {
  if (std::holds_alternative<int>(v)) {
    return std::get<int>(v);
  }
  if (std::holds_alternative<double>(v)) {
    return std::get<double>(v);
  }
  throw std::runtime_error("Expected numeric value, got: " +
                           value_to_string(v));
}

template <typename InnerType>
bool both_are_type(const Value &a, const Value &b) {
  return std::holds_alternative<InnerType>(a) &&
         std::holds_alternative<InnerType>(b);
}

template <typename InnerType> bool is_type(const Value &value) {
  return std::holds_alternative<InnerType>(value);
}

bool is_numeric(const Value &value) {
  return is_type<int>(value) || is_type<double>(value);
}

//****************************************************************************
} // namespace
//****************************************************************************

std::shared_ptr<ValueEnvironment> default_value_environment() {
  auto env = std::make_shared<ValueEnvironment>();

  // Expect arity and typing to be handled earlier than here
  // Permissive for now

  env->define("+",
              BuiltInFunction{
                  .m_name = "+",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (both_are_type<int>(args[0], args[1])) {
                      return std::get<int>(args[0]) + std::get<int>(args[1]);
                    }
                    return as_number(args[0]) + as_number(args[1]);
                  }});

  env->define("-",
              BuiltInFunction{
                  .m_name = "-",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (both_are_type<int>(args[0], args[1])) {
                      return std::get<int>(args[0]) - std::get<int>(args[1]);
                    }
                    return as_number(args[0]) - as_number(args[1]);
                  }});

  env->define("*",
              BuiltInFunction{
                  .m_name = "*",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (both_are_type<int>(args[0], args[1])) {
                      return std::get<int>(args[0]) * std::get<int>(args[1]);
                    }
                    return as_number(args[0]) * as_number(args[1]);
                  }});

  env->define("/",
              BuiltInFunction{
                  .m_name = "/",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (both_are_type<int>(args[0], args[1])) {
                      auto divisor = std::get<int>(args[1]);
                      if (divisor == 0) {
                        throw std::runtime_error("Cannot divide by 0!");
                      }
                      return std::get<int>(args[0]) / divisor;
                    }
                    return as_number(args[0]) / as_number(args[1]);
                  }});

  env->define(">",
              BuiltInFunction{
                  .m_name = ">",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (is_numeric(args[0]) && is_numeric(args[1])) {
                      return as_number(args[0]) > as_number(args[1]);
                    }
                    throw std::runtime_error(
                        "Can only compare numeric types, not: " +
                        value_to_string(args[0]) + " and " +
                        value_to_string(args[1]));
                  }});

  env->define("<",
              BuiltInFunction{
                  .m_name = "<",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (is_numeric(args[0]) && is_numeric(args[1])) {
                      return as_number(args[0]) < as_number(args[1]);
                    }
                    throw std::runtime_error(
                        "Can only compare numeric types, not: " +
                        value_to_string(args[0]) + " and " +
                        value_to_string(args[1]));
                  }});

  env->define(
      "=", BuiltInFunction{
               .m_name = "=",
               .m_expected_arguments = 2,
               .m_native_function = [](std::vector<Value> args) -> Value {
                 if (is_numeric(args[0]) && is_numeric(args[1])) {
                   return as_number(args[0]) == as_number(args[1]);
                 }
                 if (both_are_type<bool>(args[0], args[1])) {
                   return std::get<bool>(args[0]) == std::get<bool>(args[1]);
                 }
                 if (both_are_type<std::string>(args[0], args[1])) {
                   return std::get<std::string>(args[0]) ==
                          std::get<std::string>(args[1]);
                 }
                 throw std::runtime_error("Equality expects same types, not: " +
                                          value_to_string(args[0]) + " and " +
                                          value_to_string(args[1]));
               }});

  return env;
}

std::shared_ptr<TypeEnvironment> default_type_environment() {
  auto env = std::make_shared<TypeEnvironment>();

  // Built-in functions: all typed as opaque Fn for now
  env->define("+", Type::Fn);
  env->define("-", Type::Fn);
  env->define("*", Type::Fn);
  env->define("/", Type::Fn);
  env->define(">", Type::Fn);
  env->define("<", Type::Fn);
  env->define("=", Type::Fn);

  return env;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
