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

//****************************************************************************
namespace blip {
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

//****************************************************************************
} // namespace blip
//****************************************************************************
