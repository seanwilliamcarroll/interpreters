//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the zonk pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/type_registry.hpp>
#include <zir/type_resolver.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Context {
  hir::TypeId reregister(hir::TypeId old_type_id) {
    const auto &old_type = m_type_registry.get(old_type_id);
    auto new_type_id = m_new_type_registry.intern(old_type);
    return new_type_id;
  }

  hir::TypeId find_and_register(hir::TypeId old_type_id) {
    auto resolved_type_id = m_resolver.resolve(old_type_id);
    const auto &resolved_type = m_type_registry.get(resolved_type_id);

    // FunctionType contains inner TypeIds that may themselves reference
    // unresolved type variables.  Recursively resolve them before
    // re-registering so the new registry never contains stale IDs.
    if (const auto *fn = std::get_if<hir::FunctionType>(&resolved_type)) {
      std::vector<hir::TypeId> new_params;
      new_params.reserve(fn->m_parameters.size());
      for (const auto &param_id : fn->m_parameters) {
        new_params.push_back(find_and_register(param_id));
      }
      auto new_return = find_and_register(fn->m_return_type);
      return m_new_type_registry.intern(
          hir::FunctionType{std::move(new_params), new_return});
    }

    return reregister(resolved_type_id);
  }

  hir::TypeRegistry &m_type_registry;
  TypeResolver m_resolver;
  hir::TypeRegistry m_new_type_registry{};
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
