//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the let binding monomorpher.
//*
//*
//****************************************************************************

#include <hir/instantiation_record.hpp>
#include <hir/nodes.hpp>
#include <hir/type_variable_substituter.hpp>
#include <hir/types.hpp>
#include <mono/context.hpp>
#include <mono/let_binding_monomorpher.hpp>
#include <mono/let_binding_substituter.hpp>
#include <mono/name_mangler.hpp>
#include <mono/specialization.hpp>

#include <string>
#include <unordered_map>
#include <utility>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

std::vector<hir::LetBinding> LetBindingMonomorpher::monomorph(
    const hir::LetBinding &let_binding,
    const hir::TypeSubstitution &outer_substitution) {
  // We've just been handed a let binding, we need to generate all the possible
  // future let bindings based on the instantiation records
  const auto id = let_binding.m_variable.m_id;

  auto iter = m_ctx.m_instantiation_records.find(id);
  if (iter == m_ctx.m_instantiation_records.end()) {
    // Nothing to substitute
    auto context = SubstitutionContext{
        .m_parent = m_ctx, .m_substitution_mapping = outer_substitution};
    auto output = LetBindingSubstituter{context}.substitute(let_binding);
    std::vector<hir::LetBinding> new_let_bindings;
    new_let_bindings.emplace_back(std::move(output));
    return new_let_bindings;
  }

  const auto &records = iter->second;

  std::vector<hir::LetBinding> new_let_bindings;
  new_let_bindings.reserve(records.size());
  for (const auto &record : records) {
    // Record the specialization in the table

    hir::TypeSubstitution combined = outer_substitution;
    auto substituter =
        hir::TypeVariableSubstituter{.m_type_arena = m_ctx.type_arena(),
                                     .m_type_unifier = m_ctx.m_type_unifier,
                                     .m_new_mapping = outer_substitution};
    for (const auto &[original_type_id, substituted_type_id] :
         record.m_substitution) {
      combined[original_type_id] = substituter.substitute(substituted_type_id);
    }

    auto new_type =
        hir::TypeVariableSubstituter{.m_type_arena = m_ctx.type_arena(),
                                     .m_type_unifier = m_ctx.m_type_unifier,
                                     .m_new_mapping = combined}
            .substitute(let_binding.m_expression.m_type);

    auto new_id = m_ctx.next_let_binding_id();
    auto new_name = Mangler(m_ctx.type_arena())
                        .mangle(let_binding.m_variable.m_name,
                                let_binding.m_variable.m_id, new_type);

    m_ctx.m_env.define(
        let_binding.m_variable.m_id, new_type,
        Specialization{.m_mangled_name = new_name, .m_new_id = new_id});

    auto context = SubstitutionContext{.m_parent = m_ctx,
                                       .m_substitution_mapping = combined};

    new_let_bindings.push_back(
        LetBindingSubstituter{context}.substitute(let_binding));
  }

  return new_let_bindings;
}

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
