//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Post-inference type resolver — wraps unifier state to
//*            provide find-only access for the ZIR lowering pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>

#include <variant>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct TypeResolver {
  explicit TypeResolver(const hir::TypeArena &type_arena,
                        hir::UnifierState unifier_state)
      : m_type_arena(type_arena), m_unifier_state(std::move(unifier_state)) {}

  [[nodiscard]]
  hir::TypeId resolve(hir::TypeId type_id) const {
    const auto &type = m_type_arena.get(type_id);
    if (!std::holds_alternative<hir::TypeVariable>(type)) {
      return type_id;
    }

    auto root = m_unifier_state.m_union_find.find(
        std::get<hir::TypeVariable>(type).m_id);

    auto iter = m_unifier_state.m_resolved_type_id.find(root);
    if (iter != m_unifier_state.m_resolved_type_id.end()) {
      return iter->second;
    }

    // Type variable was not resolved to a concrete type
    throw core::InternalCompilerError(
        "We assume all type variables should "
        "have been resolved before ZIR lowering!\nCould not resolve: " +
        m_type_arena.to_string(type_id));
  }

private:
  const hir::TypeArena &m_type_arena;
  const hir::UnifierState m_unifier_state;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
