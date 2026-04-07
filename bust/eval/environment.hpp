//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Runtime environment for bust tree-walking evaluator.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "eval/values.hpp"
#include <memory>
#include <optional>
#include <ranges>
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
    auto iter = m_name_to_expression.find(name);
    if (iter != m_name_to_expression.end()) {
      m_name_to_expression.erase(iter);
    }
    m_name_to_expression.emplace(name, std::move(value));
  }

  std::shared_ptr<Scope> m_parent_scope;
  std::unordered_map<std::string, Value> m_name_to_expression{};
};

struct Environment {

  Environment() : m_scopes{std::make_shared<Scope>()} {}

  Environment(const std::shared_ptr<Scope> &scope) : m_scopes{scope} {}

  void push_scope() { m_scopes.emplace_back(m_scopes.back()); }

  void pop_scope() {
    if (m_scopes.size() == 1) {
      throw std::runtime_error("Cannot pop scope, already at global scope!");
    }
    m_scopes.pop_back();
  }

  std::optional<Value> lookup(const std::string &name) {
    return m_scopes.back()->lookup(name);
  }

  void define(const std::string &name, Value value) {
    m_scopes.back()->define(name, std::move(value));
  }

  std::shared_ptr<Scope> get_current_scope() const { return m_scopes.back(); }

private:
  std::vector<std::shared_ptr<Scope>> m_scopes;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
