//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the ZIR lowering pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>
#include <hir/type_registry.hpp>
#include <hir/unifier_state.hpp>
#include <zir/arena.hpp>
#include <zir/environment.hpp>
#include <zir/nodes.hpp>
#include <zir/type_resolver.hpp>

#include <unordered_map>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Context {
  explicit Context(const hir::TypeRegistry &type_registry,
                   hir::UnifierState unifier_state)
      : m_type_registry(type_registry),
        m_resolver(type_registry, std::move(unifier_state)) {}

  TypeId convert(const hir::TypeKind &type) {
    auto type_id = m_type_registry.intern(type);
    return convert(type_id);
  }

  TypeId convert(hir::TypeId type) {
    auto resolved_type_id = m_resolver.resolve(type);
    return m_arena.convert(resolved_type_id, m_type_registry);
  }

  void set_global_binding(const std::string &name, BindingId id) {
    m_global_bindings[name] = id;
  }

  [[nodiscard]] BindingId get_global_binding(const std::string &name) const {
    auto iter = m_global_bindings.find(name);
    if (iter == m_global_bindings.end()) {
      throw core::InternalCompilerError("Could not find global binding: " +
                                        name);
    }
    return iter->second;
  }

  [[nodiscard]] const std::unordered_map<std::string, BindingId> &
  global_bindings() const {
    return m_global_bindings;
  }
  Environment &env() { return m_env; }
  Arena &arena() { return m_arena; }

  [[nodiscard]] std::string to_string(hir::TypeId type) const {
    return m_type_registry.to_string(type);
  }

private:
  const hir::TypeRegistry &m_type_registry;
  TypeResolver m_resolver;
  Arena m_arena;
  Environment m_env;
  std::unordered_map<std::string, BindingId> m_global_bindings;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
