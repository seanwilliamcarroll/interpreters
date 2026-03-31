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

Environment::Environment(std::shared_ptr<Environment> parent)
    : m_parent(std::move(parent)) {}

void Environment::define(const std::string &name, Value value) {
  // Throw if already defined? We've made a distinction between defining and
  // setting

  // This is allowing shadowing by not looking at parent

  auto iter = m_bindings.find(name);
  if (iter != m_bindings.end()) {
    // Not sure how to do location here, seems out of place?
    throw std::runtime_error("Already defined: \"" + name + "\"");
  }
  m_bindings[name] = std::move(value);
}

Value Environment::lookup(const std::string &name) const {
  // TODO: search current scope, then walk parent chain
  // throw if not found
  auto iter = m_bindings.find(name);
  if (iter != m_bindings.end()) {
    return iter->second;
  }
  if (m_parent != nullptr) {
    return m_parent->lookup(name);
  }
  throw std::runtime_error("Couldn't find: \"" + name + "\"");
}

void Environment::set(const std::string &name, Value value) {
  // TODO: find existing binding in chain and mutate it
  // throw if not found
  auto iter = m_bindings.find(name);
  if (iter != m_bindings.end()) {
    iter->second = std::move(value);
    return;
  }
  if (m_parent != nullptr) {
    m_parent->set(name, std::move(value));
    return;
  }
  throw std::runtime_error("Cannot set: \"" + name + "\"");
}

std::shared_ptr<Environment> default_global_environment() {
  auto env = std::make_shared<Environment>();

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

//****************************************************************************
} // namespace blip
//****************************************************************************
