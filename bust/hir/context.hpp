//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the type checker pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/instantiation_record.hpp"
#include "hir/type_registry.hpp"
#include "hir/types.hpp"
#include "hir/unifier_state.hpp"
#include <algorithm>
#include <hir/environment.hpp>
#include <hir/type_unifier.hpp>
#include <hir/type_variable_substituter.hpp>
#include <set>
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct PostTypeCheckedData {
  TypeRegistry m_type_registry;
  UnifierState m_unifier_state;
  BindingIdInstantiations m_instantiation_records;
  InnerTypeBindingId m_next_let_binding_id;
};

struct Context {

  TypeId create_fresh_type_vars(BindingId id, const TypeScheme &type_scheme) {
    TypeSubstitution new_mapping;
    for (const auto &old_type_id : type_scheme.m_free_type_variables) {
      auto possible_type_class = m_type_unifier.find_type_class(old_type_id);
      new_mapping.emplace(old_type_id,
                          m_type_unifier.new_type_var(possible_type_class));
    }

    if (!new_mapping.empty()) {
      // We're instantiating a polymorphic lambda (?)
      m_instantiation_records[id].push_back(InstantiationRecord{new_mapping});
    }

    return TypeVariableSubstituter{m_type_registry, m_type_unifier, new_mapping}
        .substitute(type_scheme.m_type);
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

  BindingIdInstantiations resolve_instantiation_records() {
    std::unordered_map<BindingId,
                       std::set<std::vector<std::pair<TypeId, TypeId>>>>
        seen;
    BindingIdInstantiations output;

    const auto original_records = std::move(m_instantiation_records);
    m_instantiation_records.clear();

    for (const auto &[id, records] : original_records) {
      for (const auto &instantiation : records) {
        std::vector<std::pair<TypeId, TypeId>> canonical_mapping;
        const auto &mapping = instantiation.m_substitution;
        for (const auto &[original_free_type_var, instantiated_type_var] :
             mapping) {
          auto resolved_type_var_id =
              m_type_unifier.find(instantiated_type_var);
          canonical_mapping.emplace_back(original_free_type_var,
                                         resolved_type_var_id);
        }

        // Should sort a vector of tuples by first element
        std::ranges::sort(canonical_mapping);

        if (seen[id].insert(canonical_mapping).second) {
          output[id].emplace_back(InstantiationRecord{
              .m_substitution = TypeSubstitution(canonical_mapping.begin(),
                                                 canonical_mapping.end()),
          });
        }
      }
    }

    return output;
  }

  PostTypeCheckedData get_post_check_data() {
    auto instantiation_records = resolve_instantiation_records();
    auto unifier_state = m_type_unifier.extract_state();
    return {
        .m_type_registry = std::move(m_type_registry),
        .m_unifier_state = std::move(unifier_state),
        .m_instantiation_records = std::move(instantiation_records),
        .m_next_let_binding_id = m_next_let_binding_id,
    };
  }

  BindingId next_let_binding_id() { return {m_next_let_binding_id++}; }

  Environment m_env{};
  TypeRegistry m_type_registry{};
  std::vector<TypeId> m_return_type_stack{};
  TypeUnifier m_type_unifier{m_type_registry};
  BindingIdInstantiations m_instantiation_records{};
  InnerTypeBindingId m_next_let_binding_id = 0;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
