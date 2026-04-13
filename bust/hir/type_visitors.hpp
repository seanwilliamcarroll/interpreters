//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type visitors for bust HIR types (substitution, collection).
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/type_registry.hpp"
#include <hir/types.hpp>
#include <unordered_map>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeVariableUpdater {

  TypeId update(const TypeKind &type) { return std::visit(*this, type); }

  TypeId update(const TypeId &type) {
    return update(m_type_registry.get(type));
  }

  TypeId operator()(const PrimitiveTypeValue &type) {
    return m_type_registry.intern(type);
  }

  TypeId operator()(const TypeVariable &type) {
    auto iter = m_new_mapping.find(type);

    if (iter == m_new_mapping.end()) {
      return m_type_registry.intern(type);
    }

    return m_type_registry.intern(iter->second);
  }

  TypeId operator()(const FunctionType &type) {
    std::vector<TypeId> parameters;
    parameters.reserve(type.m_parameters.size());
    for (const auto &parameter : type.m_parameters) {
      parameters.emplace_back(update(m_type_registry.get(parameter)));
    }

    return m_type_registry.intern(
        FunctionType{std::move(parameters),
                     update(m_type_registry.get(type.m_return_type))});
  }

  TypeId operator()(const NeverType &) { return m_type_registry.m_never; }

  TypeRegistry &m_type_registry;
  const std::unordered_map<TypeVariable, TypeVariable> &m_new_mapping;
};

struct FreeTypeVariableCollector {

  void collect(const TypeKind &type) { std::visit(*this, type); }
  void collect(const TypeId &type) { collect(m_type_registry.get(type)); }

  void operator()(const PrimitiveTypeValue &) {}

  void operator()(const TypeVariable &type) {
    m_free_type_variables.emplace_back(type);
  }

  void operator()(const FunctionType &type) {
    for (const auto &parameter : type.m_parameters) {
      collect(m_type_registry.get(parameter));
    }

    collect(m_type_registry.get(type.m_return_type));
  }

  void operator()(const NeverType &) {}

  TypeRegistry &m_type_registry;
  std::vector<TypeVariable> m_free_type_variables{};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
