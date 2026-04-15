//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Post-inference type resolver — wraps unifier state to
//*            provide find-only access for the zonk pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "exceptions.hpp"
#include <hir/type_registry.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>
#include <variant>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct TypeResolver {
  hir::TypeId resolve(hir::TypeId type_id) {
    const auto &type = m_type_registry.get(type_id);
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
        "have been resolved before zonking!\nCould not resolve: " +
        m_type_registry.to_string(type_id));
  }

  hir::TypeRegistry &m_type_registry;
  hir::UnifierState m_unifier_state;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
