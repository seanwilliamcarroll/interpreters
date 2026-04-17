//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the ZIR lowering pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/type_registry.hpp>
#include <zir/arena.hpp>
#include <zir/environment.hpp>
#include <zir/type_resolver.hpp>

#include <unordered_map>

#include "zir/nodes.hpp"

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Context {
  TypeId convert(const hir::TypeKind &type) {
    auto type_id = m_type_registry.intern(type);
    return convert(type_id);
  }

  TypeId convert(hir::TypeId type) {
    auto resolved_type_id = m_resolver.resolve(type);
    return m_arena.convert(resolved_type_id, m_type_registry);
  }

  // TODO Make private with convenience accessors, explicit ctor
  hir::TypeRegistry &m_type_registry;
  TypeResolver m_resolver;
  Arena m_arena{};
  Environment m_env;
  std::unordered_map<std::string, BindingId> m_global_bindings;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
