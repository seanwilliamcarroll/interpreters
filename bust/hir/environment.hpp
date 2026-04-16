//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type environment for bust type checker.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <scope_guard.hpp>

#include <cassert>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeScheme {
  TypeId m_type;
  std::vector<TypeId> m_free_type_variables;
};

struct BoundTypeScheme {
  BindingId m_id;
  TypeScheme m_type_scheme;
};

struct Scope {
  // Should be a mapping of identifiers to types
  // Shadowing means we can reuse an identifier with a different type if
  // desired, meaning this is not an error. We're not reassigning, we're
  // shadowing an immutable identifier

  std::optional<BoundTypeScheme> lookup(const std::string &name) {
    auto iter = m_identifier_to_type.find(name);
    if (iter == m_identifier_to_type.end()) {
      return {};
    }
    return {iter->second};
  }

  void define(const std::string &name, BindingId id, TypeId type_id) {
    m_identifier_to_type.insert_or_assign(name,
                                          BoundTypeScheme{id, {type_id, {}}});
  }

  void define(const std::string &name, BindingId id, TypeScheme type_scheme) {
    m_identifier_to_type.insert_or_assign(
        name, BoundTypeScheme{id, std::move(type_scheme)});
  }

private:
  std::unordered_map<std::string, BoundTypeScheme> m_identifier_to_type;
};

struct Environment {
  // Need to keep track of the identifiers defined in this env, while allowing
  // for shadowing
  // Should be a stack of scopes

  // Search from top of stack until bottom and search global scope last
  // Allows us to shadow variables within a new scope

  Environment() { m_scopes.emplace_back(); }

  void push_scope() { m_scopes.emplace_back(); }
  void pop_scope() noexcept {
    assert(m_scopes.size() > 1 && "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  std::optional<BoundTypeScheme> lookup(const std::string &name) {
    for (auto &scope : m_scopes | std::views::reverse) {
      auto maybe_type = scope.lookup(name);
      if (maybe_type.has_value()) {
        return maybe_type;
      }
    }
    return {};
  }

  void define(const std::string &name, BindingId id, TypeId type_id) {
    m_scopes.back().define(name, id, type_id);
  }

  void define(const std::string &name, BindingId id, TypeScheme type_scheme) {
    m_scopes.back().define(name, id, std::move(type_scheme));
  }

private:
  std::vector<Scope> m_scopes{};
};

using ScopeGuard = core::ScopeGuard<Environment>;

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
