//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Collapse unified type variables to their root representative.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/context.hpp>
#include <hir/type_folder.hpp>
#include <hir/types.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

inline TypeId collapse_types(TypeArena &type_arena, TypeUnifier &type_unifier,
                             TypeId type_id) {
  auto type_variable_collapser =
      [&](const TypeVariable &type_variable) -> TypeId {
    auto resolved_type_id = type_unifier.find(type_variable);
    return resolved_type_id;
  };
  return fold(type_arena, type_id, type_variable_collapser);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
