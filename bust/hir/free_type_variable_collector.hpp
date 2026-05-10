//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Free type variable collection visitor for bust HIR types.
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

inline std::vector<TypeId> collect_free_variables(hir::Context &ctx,
                                                  TypeId type_id) {
  std::vector<TypeId> free_type_variables;

  auto collect_from_type_variable = [&](const TypeVariable &type_variable) {
    auto resolved_type_id = ctx.type_unifier().find(type_variable);
    if (!ctx.is_type_variable(resolved_type_id)) {
      return;
    }
    free_type_variables.emplace_back(resolved_type_id);
  };

  walk(ctx.type_arena(), type_id, collect_from_type_variable);

  return free_type_variables;
}

inline std::vector<TypeId> collect_free_variables(hir::Context &ctx,
                                                  const TypeKind &type) {
  return collect_free_variables(ctx, ctx.type_arena().intern(type));
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
