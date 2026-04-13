//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Runtime environment for bust tree-walking evaluator.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cassert>
#include <eval/values.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <scope_guard.hpp>
#include <string>
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct Scope {

  Scope(const std::shared_ptr<Scope> &parent_scope = nullptr)
      : m_parent_scope(parent_scope) {}

  std::optional<Value> lookup(const std::string &name) {
    auto iter = m_name_to_expression.find(name);
    if (iter == m_name_to_expression.end()) {
      if (m_parent_scope != nullptr) {
        return m_parent_scope->lookup(name);
      }
      return {};
    }
    return {iter->second};
  }

  void define(const std::string &name, Value value) {
    m_name_to_expression[name] = std::move(value);
  }

  std::shared_ptr<Scope> m_parent_scope;
  std::unordered_map<std::string, Value> m_name_to_expression{};
};

struct Environment {

  Environment() : m_scope{std::make_shared<Scope>()} {}

  Environment(const std::shared_ptr<Scope> &scope) : m_scope{scope} {}

  void push_scope() { m_scope = std::make_shared<Scope>(m_scope); }

  void pop_scope() noexcept {
    assert(m_scope->m_parent_scope != nullptr &&
           "Cannot pop scope, already at global scope!");
    m_scope = m_scope->m_parent_scope;
  }

  std::optional<Value> lookup(const std::string &name) {
    return m_scope->lookup(name);
  }

  void define(const std::string &name, Value value) {
    m_scope->define(name, std::move(value));
  }

  std::shared_ptr<Scope> get_current_scope() const { return m_scope; }

private:
  std::shared_ptr<Scope> m_scope;
};

using ScopeGuard = core::ScopeGuard<Environment>;

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
