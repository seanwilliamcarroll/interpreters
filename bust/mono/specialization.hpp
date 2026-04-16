//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose :
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************
#include <hash_combine.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/types.hpp>
#include <scope_guard.hpp>

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct Specialization {
  std::string m_mangled_name;
  hir::BindingId m_new_id;
};

struct ScopeKey {
  hir::BindingId m_id;
  hir::TypeId m_type_id;

  auto operator<=>(const ScopeKey &) const = default;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
//****************************************************************************
namespace std {
//****************************************************************************

template <> struct hash<bust::mono::ScopeKey> {
  size_t operator()(const bust::mono::ScopeKey &key) const {
    size_t seed = 0;
    core::hash_combine(seed, key.m_id);
    core::hash_combine(seed, key.m_type_id);
    return seed;
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
//****************************************************************************
namespace bust::mono {
//****************************************************************************

using SpecializationTable =
    std::unordered_map<ScopeKey, std::unique_ptr<Specialization>>;

struct Scope {

  Scope(const std::shared_ptr<Scope> &parent) : m_parent_scope(parent) {}

  const Specialization *lookup(hir::BindingId id, hir::TypeId type_id) {
    // Given an original let binding and computed typeid, find the
    // specialization if it exists at this scope, else look to parents
    auto key = ScopeKey{id, type_id};
    return lookup(key);
  }

  const Specialization *lookup(const ScopeKey &key) {
    auto iter = m_table.find(key);
    if (iter == m_table.end()) {
      if (m_parent_scope != nullptr) {
        return m_parent_scope->lookup(key);
      }
      return nullptr;
    }
    return iter->second.get();
  }

  void define(hir::BindingId id, hir::TypeId type_id,
              Specialization specialization) {
    auto key = ScopeKey{id, type_id};
    m_table.insert_or_assign(
        key, std::make_unique<Specialization>(std::move(specialization)));
  }

  std::shared_ptr<Scope> m_parent_scope;
  SpecializationTable m_table;
};

struct Environment {
  // Probably could generalize at some point across Environments

  Environment() { m_scopes.emplace_back(std::make_shared<Scope>(nullptr)); }

  void push_scope() {
    m_scopes.emplace_back(std::make_shared<Scope>(m_scopes.back()));
  }
  void pop_scope() noexcept {
    assert(m_scopes.size() > 1 && "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  const Specialization *lookup(hir::BindingId id, hir::TypeId type_id) {
    return m_scopes.back()->lookup(id, type_id);
  }

  void define(hir::BindingId id, hir::TypeId type_id,
              Specialization specialization) {
    m_scopes.back()->define(id, type_id, std::move(specialization));
  }

private:
  std::vector<std::shared_ptr<Scope>> m_scopes{};
};

using ScopeGuard = core::ScopeGuard<Environment>;

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
