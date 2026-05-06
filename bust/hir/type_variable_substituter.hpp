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
#include <hir/type_folder.hpp>
#include <hir/type_unifier.hpp>
#include <hir/types.hpp>

#include <unordered_map>
#include <variant>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

inline TypeId
substitute_types(TypeArena &type_arena, TypeUnifier &type_unifier,
                 TypeId type_id,
                 const std::unordered_map<TypeId, TypeId> &new_mapping) {
  auto type_variable_substituter =
      [&](const TypeVariable &type_variable) -> TypeId {
    auto type_id = type_arena.intern(type_variable);
    auto iter = new_mapping.find(type_id);
    if (iter == new_mapping.end()) {
      auto resolved_type_id = type_unifier.find(type_variable);
      auto resolved_iter = new_mapping.find(resolved_type_id);
      if (resolved_iter != new_mapping.end()) {
        return resolved_iter->second;
      }
      if (type_arena.is_type_variable(resolved_type_id)) {
        return resolved_type_id;
      }
      // Otherwise, recurse on it?
      return substitute_types(type_arena, type_unifier, resolved_type_id,
                              new_mapping);
    }
    return iter->second;
  };
  return fold(type_arena, type_id, type_variable_substituter);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
