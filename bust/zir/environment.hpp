//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Scoped environment for ZIR lowering.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************
#include <scope_guard.hpp>
#include <zir/nodes.hpp>

#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>

// Same old env structure, scopes with parent pointers, map name to id in
// closest scope
// Easy because all type checked and monomorphed
// Really need to align all these envs at some point

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Scope {
  Scope(const std::shared_ptr<Scope> &parent) : m_parent_scope(parent) {}

  std::optional<BindingId> lookup(const std::string &name) {
    auto iter = m_table.find(name);
    if (iter == m_table.end()) {
      if (m_parent_scope != nullptr) {
        return m_parent_scope->lookup(name);
      }
      return {};
    }
    return std::make_optional(iter->second);
  }

  void define(const std::string &name, BindingId id) { m_table[name] = id; }

  std::shared_ptr<Scope> m_parent_scope;
  std::unordered_map<std::string, BindingId> m_table;
};

struct Environment {
  // Probably could generalize at some point across Environments

  Environment() { m_scopes.emplace_back(std::make_shared<Scope>(nullptr)); }

  void push_scope() {
    m_scopes.emplace_back(std::make_shared<Scope>(m_scopes.back()));
  }
  void pop_scope() noexcept {
    assert(m_scopes.size() > 1 && "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  std::optional<BindingId> lookup(const std::string &name) {
    return m_scopes.back()->lookup(name);
  }

  void define(const std::string &name, BindingId id) {
    m_scopes.back()->define(name, id);
  }

private:
  std::vector<std::shared_ptr<Scope>> m_scopes{};
};

using ScopeGuard = core::ScopeGuard<Environment>;

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
