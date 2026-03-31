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

bool both_int(const Value &a, const Value &b) {
  return std::holds_alternative<int>(a) && std::holds_alternative<int>(b);
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

  env->define("+",
              BuiltInFunction{
                  .m_name = "+",
                  .m_expected_arguments = 2,
                  .m_native_function = [](std::vector<Value> args) -> Value {
                    if (both_int(args[0], args[1])) {
                      return std::get<int>(args[0]) + std::get<int>(args[1]);
                    }
                    return as_number(args[0]) + as_number(args[1]);
                  }});

  return env;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
