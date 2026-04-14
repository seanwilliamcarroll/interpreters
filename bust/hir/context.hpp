//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the type checker pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/type_registry.hpp"
#include "hir/types.hpp"
#include <algorithm>
#include <hir/environment.hpp>
#include <hir/type_unifier.hpp>
#include <hir/type_variable_updater.hpp>
#include <map>
#include <set>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

using TypeSubstitution = std::unordered_map<TypeId, TypeId>;

struct InstantiationRecord {
  // Helper to keep track of each time a polymorphic lambda is instantiated for
  // monomorphization later
  std::string m_let_binding;
  // Need to map the original free type variables to the new ones created for
  // this instantiation
  TypeSubstitution m_substitution;
};

struct Context {

  TypeId create_fresh_type_vars(const std::string &bound_name,
                                const TypeScheme &type_scheme) {
    TypeSubstitution new_mapping;
    for (const auto &old_type_id : type_scheme.m_free_type_variables) {
      auto possible_type_class = m_type_unifier.find_type_class(old_type_id);
      new_mapping.emplace(old_type_id,
                          m_type_unifier.new_type_var(possible_type_class));
    }

    if (!new_mapping.empty()) {
      // We're instantiating a polymorphic lambda (?)
      m_instantiation_records.push_back(
          InstantiationRecord{bound_name, new_mapping});
    }

    return TypeVariableUpdater{m_type_registry, new_mapping}.update(
        type_scheme.m_type);
  }

  const FunctionType &as_function(TypeId type_id) const {
    return m_type_registry.as_function(type_id);
  }

  const PrimitiveTypeValue &as_primitive(TypeId type_id) const {
    return m_type_registry.as_primitive(type_id);
  }

  const TypeVariable &as_type_variable(TypeId type_id) const {
    return m_type_registry.as_type_variable(type_id);
  }

  bool is_function(TypeId type_id) const {
    return m_type_registry.is_function(type_id);
  }

  bool is_primitive(TypeId type_id) const {
    return m_type_registry.is_primitive(type_id);
  }

  bool is_type_variable(TypeId type_id) const {
    return m_type_registry.is_type_variable(type_id);
  }

  std::string to_string(const auto &type) const {
    return m_type_registry.to_string(type);
  }

  std::vector<InstantiationRecord> resolve_instantiation_records() {
    std::unordered_map<std::string,
                       std::set<std::vector<std::pair<TypeId, TypeId>>>>
        seen;
    std::vector<InstantiationRecord> output;

    const auto original_records = std::move(m_instantiation_records);
    m_instantiation_records.clear();

    for (const auto &record : original_records) {
      const auto &name = record.m_let_binding;
      const auto &mapping = record.m_substitution;

      std::vector<std::pair<TypeId, TypeId>> canonical_mapping;
      for (const auto &[original_free_type_var, instantiated_type_var] :
           mapping) {
        auto resolved_type_var_id = m_type_unifier.find(instantiated_type_var);
        canonical_mapping.emplace_back(original_free_type_var,
                                       resolved_type_var_id);
      }

      // Should sort a vector of tuples by first element
      std::ranges::sort(canonical_mapping);

      if (seen[name].insert(canonical_mapping).second) {
        output.emplace_back(InstantiationRecord{
            .m_let_binding = name,
            .m_substitution = TypeSubstitution(canonical_mapping.begin(),
                                               canonical_mapping.end()),
        });
      }
    }

    return output;
  }

  Environment &m_env;
  TypeRegistry &m_type_registry;
  std::vector<TypeId> m_return_type_stack{};
  TypeUnifier m_type_unifier{m_type_registry};
  std::vector<InstantiationRecord> m_instantiation_records{};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
