//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Lexically-scoped environment for blip evaluation
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "type.hpp"
#include <memory>
#include <string>
#include <unordered_map>

#include <value.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

template <typename InnerType> class Environment {
public:
  explicit Environment(std::shared_ptr<Environment> parent = nullptr)
      : m_parent(std::move(parent)) {}

  void define(const std::string &name, InnerType value) {
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

  InnerType lookup(const std::string &name) const {
    auto iter = m_bindings.find(name);
    if (iter != m_bindings.end()) {
      return iter->second;
    }
    if (m_parent != nullptr) {
      return m_parent->lookup(name);
    }
    throw std::runtime_error("Couldn't find: \"" + name + "\"");
  }

  void set(const std::string &name, InnerType value) {
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

private:
  std::unordered_map<std::string, InnerType> m_bindings;
  std::shared_ptr<Environment> m_parent;
};

class ValueEnvironment : public Environment<Value> {
public:
  ValueEnvironment(std::shared_ptr<ValueEnvironment> parent = nullptr)
      : Environment<Value>(std::move(parent)) {}
};

class TypeEnvironment : public Environment<Type> {
public:
  TypeEnvironment(std::shared_ptr<TypeEnvironment> parent = nullptr)
      : Environment<Type>(std::move(parent)) {}
};

std::shared_ptr<ValueEnvironment> default_value_environment();

std::shared_ptr<TypeEnvironment> default_type_environment();

//****************************************************************************
} // namespace blip
//****************************************************************************
