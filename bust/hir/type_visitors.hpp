//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type visitors for bust HIR types (substitution, collection).
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/types.hpp>
#include <unordered_map>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeVariableUpdater {
  Type operator()(const PrimitiveTypeValue &type) { return type; }

  Type operator()(const TypeVariable &type) {
    auto iter = m_new_mapping.find(type);

    if (iter == m_new_mapping.end()) {
      return type;
    }

    return iter->second;
  }

  Type operator()(const FunctionTypePtr &type) {
    std::vector<Type> parameter_types;
    parameter_types.reserve(type->m_argument_types.size());
    for (const auto &parameter_type : type->m_argument_types) {
      parameter_types.emplace_back(std::visit(*this, parameter_type));
    }

    return std::make_shared<FunctionTypePtr::element_type>(
        FunctionType{{type->m_location},
                     std::move(parameter_types),
                     std::visit(*this, type->m_return_type)});
  }

  Type operator()(const NeverType &type) { return type; }

  const std::unordered_map<TypeVariable, TypeVariable> &m_new_mapping;
};

struct FreeTypeVariableCollector {
  void operator()(const PrimitiveTypeValue &) {}

  void operator()(const TypeVariable &type) {
    m_free_type_variables.emplace_back(type);
  }

  void operator()(const FunctionTypePtr &type) {
    for (const auto &parameter_type : type->m_argument_types) {
      std::visit(*this, parameter_type);
    }

    std::visit(*this, type->m_return_type);
  }

  void operator()(const NeverType &) {}

  std::vector<TypeVariable> m_free_type_variables;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
