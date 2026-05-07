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

#include <algorithm>
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

struct Binding {
  BindingId m_id;
  TypeScheme m_type_scheme;
  bool m_is_mutable = false;
};

struct Scope {
  // Should be a mapping of identifiers to types
  // Shadowing means we can reuse an identifier with a different type if
  // desired, meaning this is not an error. We're not reassigning, we're
  // shadowing an immutable identifier

  std::optional<Binding> lookup(const std::string &name) {
    auto iter = m_identifier_to_type.find(name);
    if (iter == m_identifier_to_type.end()) {
      return {};
    }
    return {iter->second};
  }

  void define(const std::string &name, BindingId id, TypeId type_id,
              bool is_mutable) {
    m_identifier_to_type.insert_or_assign(
        name, Binding{
                  .m_id = id,
                  .m_type_scheme =
                      {
                          .m_type = type_id,
                          .m_free_type_variables = {},
                      },
                  .m_is_mutable = is_mutable,
              });
  }

  void define(const std::string &name, BindingId id, TypeScheme type_scheme,
              bool is_mutable) {
    m_identifier_to_type.insert_or_assign(
        name, Binding{
                  .m_id = id,
                  .m_type_scheme = std::move(type_scheme),
                  .m_is_mutable = is_mutable,
              });
  }

private:
  std::unordered_map<std::string, Binding> m_identifier_to_type;
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

  std::optional<Binding> lookup(const std::string &name) {
    for (auto &scope : m_scopes | std::views::reverse) {
      auto maybe_type = scope.lookup(name);
      if (maybe_type.has_value()) {
        return maybe_type;
      }
    }
    return {};
  }

  void define(const std::string &name, BindingId id, TypeId type_id,
              bool is_mutable) {
    m_scopes.back().define(name, id, type_id, is_mutable);
  }

  void define(const std::string &name, BindingId id, TypeScheme type_scheme,
              bool is_mutable) {
    m_scopes.back().define(name, id, std::move(type_scheme), is_mutable);
  }

private:
  std::vector<Scope> m_scopes;
};

using ScopeGuard = core::ScopeGuard<Environment>;

struct LoopEnvironment {
  enum class ScopeState : uint8_t {
    FUNCTION_TOP,
    IN_LOOP,
  };

  LoopEnvironment() { m_scopes.push_back(ScopeState::FUNCTION_TOP); }

  void push_scope(ScopeState new_scope_state) {
    m_scopes.push_back(new_scope_state);
  }
  void pop_scope() noexcept {
    assert(m_scopes.size() > 1 && "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  [[nodiscard]] bool is_in_loop_scope() const {
    return m_scopes.back() == ScopeState::IN_LOOP;
  }

private:
  std::vector<ScopeState> m_scopes;
};

template <LoopEnvironment::ScopeState AddedState>
struct LoopEnvironmentScopeGuard {
  explicit LoopEnvironmentScopeGuard(LoopEnvironment &scoped)
      : m_scoped(scoped) {
    m_scoped.push_scope(AddedState);
  }
  ~LoopEnvironmentScopeGuard() { m_scoped.pop_scope(); }

  LoopEnvironmentScopeGuard(const LoopEnvironmentScopeGuard &) = delete;
  LoopEnvironmentScopeGuard &
  operator=(const LoopEnvironmentScopeGuard &) = delete;
  LoopEnvironmentScopeGuard(LoopEnvironmentScopeGuard &&) = delete;
  LoopEnvironmentScopeGuard &operator=(LoopEnvironmentScopeGuard &&) = delete;

private:
  LoopEnvironment &m_scoped;
};

using FunctionContextScopeGuard =
    LoopEnvironmentScopeGuard<LoopEnvironment::ScopeState::FUNCTION_TOP>;
using LoopContextScopeGuard =
    LoopEnvironmentScopeGuard<LoopEnvironment::ScopeState::IN_LOOP>;

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
