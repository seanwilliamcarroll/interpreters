//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type environment for bust type checker.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/types.hpp"
#include <hir/nodes.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeVariableUpdater {
  Type operator()(const PrimitiveTypeValue &type) { return clone_type(type); }

  Type operator()(const TypeVariable &type) {
    auto iter = m_new_mapping.find(type);

    if (iter == m_new_mapping.end()) {
      return clone_type(type);
    }

    return clone_type(iter->second);
  }

  Type operator()(const std::unique_ptr<FunctionType> &type) {
    std::vector<Type> parameter_types;
    parameter_types.reserve(type->m_argument_types.size());
    for (const auto &parameter_type : type->m_argument_types) {
      parameter_types.emplace_back(std::visit(*this, parameter_type));
    }

    return std::make_unique<FunctionType>(
        FunctionType{{type->m_location},
                     std::move(parameter_types),
                     std::visit(*this, type->m_return_type)});
  }

  Type operator()(const NeverType &type) { return clone_type(type); }

  const std::unordered_map<TypeVariable, TypeVariable> &m_new_mapping;
};

struct FreeTypeVariableCollector {
  void operator()(const PrimitiveTypeValue &) {}

  void operator()(const TypeVariable &type) {
    m_free_type_variables.emplace_back(type);
  }

  void operator()(const std::unique_ptr<FunctionType> &type) {
    for (const auto &parameter_type : type->m_argument_types) {
      std::visit(*this, parameter_type);
    }

    std::visit(*this, type->m_return_type);
  }

  void operator()(const NeverType &) {}

  std::vector<TypeVariable> m_free_type_variables;
};

struct TypeScheme {
  TypeScheme(Type type, std::vector<hir::TypeVariable> free_variables)
      : m_type(std::move(type)),
        m_free_type_variables(std::move(free_variables)) {}

  TypeScheme(const TypeScheme &to_copy)
      : m_type(hir::clone_type(to_copy.m_type)),
        m_free_type_variables(to_copy.m_free_type_variables) {}

  TypeScheme(TypeScheme &&to_move) noexcept
      : m_type(std::move(to_move.m_type)),
        m_free_type_variables(std::move(to_move.m_free_type_variables)) {}

  hir::Type m_type;
  std::vector<hir::TypeVariable> m_free_type_variables;
};

struct Scope {
  // Should be a mapping of identifiers to types
  // Shadowing means we can reuse an identifier with a different type if
  // desired, meaning this is not an error. We're not reassigning, we're
  // shadowing an immutable identifier

  std::optional<TypeScheme> lookup(const std::string &name) {
    auto iter = m_identifier_to_type.find(name);
    if (iter == m_identifier_to_type.end()) {
      return {};
    }
    return {iter->second};
  }

  void define(const std::string &name, Type type) {
    m_identifier_to_type.emplace(name, TypeScheme{std::move(type), {}});
  }

  void define(const std::string &name, TypeScheme type_scheme) {
    m_identifier_to_type.emplace(name, std::move(type_scheme));
  }

private:
  std::unordered_map<std::string, TypeScheme> m_identifier_to_type;
};

struct Environment {
  // Need to keep track of the identifiers defined in this env, while allowing
  // for shadowing
  // Should be a stack of scopes

  // Search from top of stack until bottom and search global scope last
  // Allows us to shadow variables within a new scope

  Environment() { m_scopes.emplace_back(); }

  void push_scope() { m_scopes.emplace_back(); }
  void pop_scope() {
    if (m_scopes.size() == 1) {
      throw std::runtime_error("Cannot pop scope, already at global scope!");
    }
    m_scopes.pop_back();
  }

  std::optional<TypeScheme> lookup(const std::string &name) {
    for (auto &scope : m_scopes | std::views::reverse) {
      auto maybe_type = scope.lookup(name);
      if (maybe_type.has_value()) {
        return maybe_type;
      }
    }
    return {};
  }

  void define(const std::string &name, Type type) {
    m_scopes.back().define(name, std::move(type));
  }

  void define(const std::string &name, TypeScheme type_scheme) {
    m_scopes.back().define(name, std::move(type_scheme));
  }

private:
  std::vector<Scope> m_scopes{};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
