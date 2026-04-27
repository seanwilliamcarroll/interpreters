//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Symbol table for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/instructions.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <exceptions.hpp>
#include <scope_guard.hpp>

#include <ranges>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct AllocaBinding {
  Value m_ptr;
  TypeId m_internal_type_id;
};

struct ClosureBinding {
  Value m_ptr;
};

struct FunctionBinding {
  Value m_callee;
  TypeId m_return_type;
  std::vector<TypeId> m_parameter_types;
};

using Binding = std::variant<AllocaBinding, ClosureBinding, FunctionBinding>;

struct UniqueNameTracker {
  std::string uniquify(const std::string &name) {
    auto count = ++m_master_name_count[name];
    return name + "." + std::to_string(count);
  }

  std::unordered_map<std::string, size_t> m_master_name_count;
};

struct Scope {
  void define(const std::string &name, Binding binding) {
    m_symbol_to_value[name] = std::move(binding);
  }

  [[nodiscard]] std::optional<Binding> lookup(const std::string &name) const {
    auto iter = m_symbol_to_value.find(name);

    if (iter == m_symbol_to_value.end()) {
      return {};
    }

    return {iter->second};
  }

private:
  std::unordered_map<std::string, Binding> m_symbol_to_value;
};

struct SymbolTable {
  SymbolTable() : m_scopes{{}} {}

  void push_scope() { m_scopes.emplace_back(); }

  void pop_scope() noexcept {
    assert((m_scopes.size() > 1) &&
           "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  void bind_local(const std::string &name, Binding binding) {
    // This binding is associated with this name at the local scope
    m_scopes.back().define(name, std::move(binding));
  }

  void bind_global(const std::string &name, Binding binding) {
    // This binding is associated with this name at the global scope
    m_scopes.front().define(name, std::move(binding));
  }

  [[nodiscard]] Binding lookup(const std::string &name) const {
    for (const auto &scope : m_scopes | std::views::reverse) {
      auto maybe_handle = scope.lookup(name);
      if (maybe_handle.has_value()) {
        return maybe_handle.value();
      }
    }
    throw core::InternalCompilerError("symbol lookup failed for '" + name +
                                      "'");
  }

private:
  std::vector<Scope> m_scopes;
};

using ScopeGuard = core::ScopeGuard<SymbolTable>;

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
