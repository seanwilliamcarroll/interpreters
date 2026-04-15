//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type variable substitution visitor for bust HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/type_unifier.hpp"
#include <hir/type_registry.hpp>
#include <hir/types.hpp>
#include <unordered_map>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeVariableSubstituter {

  TypeId substitute(const TypeKind &type) { return std::visit(*this, type); }

  TypeId substitute(const TypeId &type) {
    return substitute(m_type_registry.get(type));
  }

  TypeId operator()(const PrimitiveTypeValue &type) {
    return m_type_registry.intern(type);
  }

  TypeId operator()(const TypeVariable &type) {
    auto type_id = m_type_registry.intern(type);
    auto iter = m_new_mapping.find(type_id);

    if (iter == m_new_mapping.end()) {
      auto resolved_type_id = m_type_unifier.find(type);
      if (m_type_registry.is_type_variable(resolved_type_id)) {
        return resolved_type_id;
      }
      // Otherwise, recurse on it?
      return substitute(resolved_type_id);
    }

    return iter->second;
  }

  TypeId operator()(const FunctionType &type) {
    std::vector<TypeId> parameters;
    parameters.reserve(type.m_parameters.size());
    for (const auto &parameter : type.m_parameters) {
      parameters.emplace_back(substitute(m_type_registry.get(parameter)));
    }

    return m_type_registry.intern(
        FunctionType{std::move(parameters),
                     substitute(m_type_registry.get(type.m_return_type))});
  }

  TypeId operator()(const NeverType &) { return m_type_registry.m_never; }

  TypeRegistry &m_type_registry;
  TypeUnifier &m_type_unifier;
  const std::unordered_map<TypeId, TypeId> &m_new_mapping;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
