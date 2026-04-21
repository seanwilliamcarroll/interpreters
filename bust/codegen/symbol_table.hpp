//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Symbol table for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/handle.hpp>
#include <codegen/instructions.hpp>
#include <exceptions.hpp>
#include <scope_guard.hpp>

#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct UniqueNameTracker {
  std::string uniquify(const std::string &name) {
    auto count = ++m_master_name_count[name];
    return name + "." + std::to_string(count);
  }

  std::unordered_map<std::string, size_t> m_master_name_count;
};

struct Scope {
  void define(const std::string &name, const Handle &handle) {
    m_symbol_to_handle[name] = handle;
  }

  [[nodiscard]] std::optional<Handle> lookup(const std::string &name) const {
    auto iter = m_symbol_to_handle.find(name);

    if (iter == m_symbol_to_handle.end()) {
      return {};
    }

    return {iter->second};
  }

private:
  std::unordered_map<std::string, Handle> m_symbol_to_handle;
};

struct SymbolTable {
  SymbolTable() : m_scopes{{}} {}

  void push_scope() { m_scopes.emplace_back(); }

  void pop_scope() noexcept {
    assert((m_scopes.size() > 1) &&
           "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  NamedHandle define_named(const std::string &name) {
    NamedHandle new_handle{m_name_tracker.uniquify(name)};
    m_scopes.back().define(name, new_handle);
    return new_handle;
  }

  GlobalHandle define_global(const std::string &name) {
    GlobalHandle new_handle{name};
    m_scopes.front().define(name, new_handle);
    return new_handle;
  }

  GlobalHandle define_custom_global(const std::string &lookup_name,
                                    const std::string &actual_name) {
    GlobalHandle new_handle{actual_name};
    m_scopes.front().define(lookup_name, new_handle);
    return new_handle;
  }

  GlobalHandle define_uniqued_global(const std::string &name) {
    return define_global(m_name_tracker.uniquify(name));
  }

  [[nodiscard]] Handle lookup(const std::string &name) const {
    for (const auto &scope : m_scopes | std::views::reverse) {
      auto maybe_handle = scope.lookup(name);
      if (maybe_handle.has_value()) {
        return maybe_handle.value();
      }
    }
    throw core::InternalCompilerError("symbol lookup failed for '" + name +
                                      "'");
  }

  Handle next_ssa_temporary() { return TemporaryHandle{m_ssa_count++}; }

private:
  std::vector<Scope> m_scopes;
  UniqueNameTracker m_name_tracker{};
  size_t m_ssa_count = 1;
};

using ScopeGuard = core::ScopeGuard<SymbolTable>;

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
