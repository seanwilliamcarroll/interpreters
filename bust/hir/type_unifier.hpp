//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type unifier for HM-style type inference using union-find.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/types.hpp"
#include "hir/union_find.hpp"
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <variant>

//****************************************************************************
namespace bust {
//****************************************************************************

struct TypeUnifier {
  hir::TypeVariable new_type_var() { return {.m_id = m_union_find.add_node()}; }

  void unify(const hir::Type &type_a, const hir::Type &type_b) {
    if (std::holds_alternative<hir::NeverType>(type_a) ||
        std::holds_alternative<hir::NeverType>(type_b)) {
      return;
    }

    if (std::holds_alternative<hir::TypeVariable>(type_a)) {
      unify(std::get<hir::TypeVariable>(type_a), type_b);
      return;
    }

    if (std::holds_alternative<hir::TypeVariable>(type_b)) {
      unify(type_a, std::get<hir::TypeVariable>(type_b));
      return;
    }

    // We've been passed two concrete types

    if (type_a == type_b) {
      return;
    }

    // First check if they are both function types
    if (std::holds_alternative<std::unique_ptr<hir::FunctionType>>(type_a) &&
        std::holds_alternative<std::unique_ptr<hir::FunctionType>>(type_b)) {
      unify(std::get<std::unique_ptr<hir::FunctionType>>(type_a),
            std::get<std::unique_ptr<hir::FunctionType>>(type_b));
      return;
    }

    throw std::runtime_error("Tried to unify concrete types: " + type_a +
                             " and " + type_b);
  }

  void unify(const std::unique_ptr<hir::FunctionType> &type_a,
             const std::unique_ptr<hir::FunctionType> &type_b) {
    if (type_a->m_argument_types.size() != type_b->m_argument_types.size()) {
      throw std::runtime_error(std::string("Tried to unify concrete types: ") +
                               hir::TypeToStringConverter{}(type_a) + " and " +
                               hir::TypeToStringConverter{}(type_b));
    }

    for (const auto &[parameter_type_a, parameter_type_b] :
         std::views::zip(type_a->m_argument_types, type_b->m_argument_types)) {
      unify(parameter_type_a, parameter_type_b);
    }

    unify(type_a->m_return_type, type_b->m_return_type);
  }

  void unify(const hir::Type &type_a, const hir::TypeVariable &type_b) {
    // Define it once in the other method
    unify(type_b, type_a);
  }

  void unify(const hir::TypeVariable &type_a, const hir::Type &type_b) {
    if (std::holds_alternative<hir::TypeVariable>(type_b)) {
      unify(type_a, std::get<hir::TypeVariable>(type_b));
      return;
    }

    if (std::holds_alternative<hir::NeverType>(type_b)) {
      return;
    }

    // We've been handled a concrete type, need to add as a resolved type

    auto root_a = m_union_find.find(type_a.m_id);

    auto iter_a = m_resolved_type.find(root_a);
    if (iter_a != m_resolved_type.end()) {
      const auto &concrete_a = iter_a->second;
      if (concrete_a != type_b) {
        throw std::runtime_error("Could not resolve type variable " + type_a +
                                 " (" + concrete_a + ") and " + type_b);
      }
      // We're good then
      return;
    }

    // No entry
    m_resolved_type.emplace(root_a, hir::clone_type(type_b));
  }

  void unify(const hir::TypeVariable &type_a, const hir::TypeVariable &type_b) {
    auto root_a = m_union_find.find(type_a.m_id);
    auto root_b = m_union_find.find(type_b.m_id);

    if (root_a == root_b) {
      // Already unified, easy
      return;
    }

    auto iter_a = m_resolved_type.find(root_a);
    auto iter_b = m_resolved_type.find(root_b);
    if (iter_a != m_resolved_type.end() && iter_b != m_resolved_type.end()) {
      // Should be a formality but check just to be sure
      const auto &concrete_a = iter_a->second;
      const auto &concrete_b = iter_b->second;
      if (concrete_a != concrete_b) {
        throw std::runtime_error("Could not resolve type variables " + type_a +
                                 " (" + concrete_a + ") and " + type_b + "(" +
                                 concrete_b + ")");
      }
      // Their concrete types must match, not sure if a bug or not
      return;
    }

    m_union_find.unite(type_a.m_id, type_b.m_id);

    if (iter_a != m_resolved_type.end() || iter_b != m_resolved_type.end()) {
      // Need to update the resolved types since someone was already concrete
      auto new_root = m_union_find.find(type_a.m_id);
      if (iter_a != m_resolved_type.end()) {
        // A is concrete, B is not
        const auto &concrete_a = iter_a->second;
        m_resolved_type.emplace(new_root, hir::clone_type(concrete_a));
      } else {
        const auto &concrete_b = iter_b->second;
        // B is concrete, A is not
        m_resolved_type.emplace(new_root, hir::clone_type(concrete_b));
      }
    }
  }

  hir::Type find(const hir::Type &type) {
    if (std::holds_alternative<hir::TypeVariable>(type)) {
      return find(std::get<hir::TypeVariable>(type));
    }
    return hir::clone_type(type);
  }

  hir::Type find(const hir::TypeVariable &type) {
    auto root = m_union_find.find(type.m_id);

    auto iter = m_resolved_type.find(root);
    if (iter != m_resolved_type.end()) {
      return hir::clone_type(iter->second);
    }

    // Not a concrete type yet
    return hir::TypeVariable{{}, root};
  }

  UnionFind m_union_find{};
  std::unordered_map<size_t, hir::Type> m_resolved_type;
};

//****************************************************************************
} // namespace bust
//****************************************************************************
