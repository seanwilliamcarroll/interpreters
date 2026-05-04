//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type variable substitution visitor for bust HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/type_arena.hpp>
#include <hir/type_unifier.hpp>
#include <hir/types.hpp>

#include <unordered_map>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeVariableSubstituter {
  TypeId substitute(TypeId type_id) {
    // Explicitly keep as copy rather than const ref so ref doesn't go stale as
    // we intern types during traversal of recursive types
    TypeKind type = m_type_arena.get(type_id);
    return std::visit(*this, type);
  }

  TypeId operator()(const PrimitiveTypeValue &type) {
    return m_type_arena.intern(type);
  }

  TypeId operator()(const TypeVariable &type) {
    auto type_id = m_type_arena.intern(type);
    auto iter = m_new_mapping.find(type_id);

    if (iter == m_new_mapping.end()) {
      auto resolved_type_id = m_type_unifier.find(type);
      auto resolved_iter = m_new_mapping.find(resolved_type_id);
      if (resolved_iter != m_new_mapping.end()) {
        return resolved_iter->second;
      }
      if (m_type_arena.is_type_variable(resolved_type_id)) {
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
      parameters.emplace_back(substitute(parameter));
    }

    return m_type_arena.intern(FunctionType{
        .m_parameters = std::move(parameters),
        .m_return_type = substitute(type.m_return_type),
    });
  }

  TypeId operator()(const TupleType &type) {
    std::vector<TypeId> fields;
    fields.reserve(type.m_fields.size());
    for (const auto &field : type.m_fields) {
      fields.emplace_back(substitute(field));
    }

    return m_type_arena.intern(TupleType{
        .m_fields = std::move(fields),
    });
  }

  TypeId operator()(const NeverType & /*unused*/) {
    return m_type_arena.m_never;
  }

  TypeArena &m_type_arena;
  TypeUnifier &m_type_unifier;
  const std::unordered_map<TypeId, TypeId> &m_new_mapping;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
