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
#include <zonk/type_resolver.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

struct Context {
  hir::TypeId reregister(hir::TypeId old_type_id) {
    const auto &old_type = m_type_registry.get(old_type_id);
    auto new_type_id = m_new_type_registry.intern(old_type);
    return new_type_id;
  }

  hir::TypeId find_and_register(hir::TypeId old_type_id) {
    auto resolved_type_id = m_resolver.resolve(old_type_id);
    return reregister(resolved_type_id);
  }

  hir::TypeRegistry &m_type_registry;
  TypeResolver m_resolver;
  hir::TypeRegistry m_new_type_registry{};
};

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
