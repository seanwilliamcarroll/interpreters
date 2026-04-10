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

#include <hir/types.hpp>
#include <hir/union_find.hpp>
#include <ranges>
#include <stdexcept>
#include <types.hpp>
#include <unordered_map>
#include <variant>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeUnifier {
  TypeVariable new_type_var() { return {.m_id = m_union_find.add_node()}; }

  void unify(const Type &type_a, const Type &type_b) {
    if (std::holds_alternative<NeverType>(type_a) ||
        std::holds_alternative<NeverType>(type_b)) {
      return;
    }

    if (std::holds_alternative<TypeVariable>(type_a)) {
      unify(std::get<TypeVariable>(type_a), type_b);
      return;
    }

    if (std::holds_alternative<TypeVariable>(type_b)) {
      unify(type_a, std::get<TypeVariable>(type_b));
      return;
    }

    // We've been passed two concrete types

    if (type_a == type_b) {
      return;
    }

    // First check if they are both function types
    if (std::holds_alternative<FunctionTypePtr>(type_a) &&
        std::holds_alternative<FunctionTypePtr>(type_b)) {
      unify(std::get<FunctionTypePtr>(type_a),
            std::get<FunctionTypePtr>(type_b));
      return;
    }

    throw std::runtime_error("Tried to unify concrete types: " + type_a +
                             " and " + type_b);
  }

  void unify(const FunctionTypePtr &type_a, const FunctionTypePtr &type_b) {
    if (type_a->m_argument_types.size() != type_b->m_argument_types.size()) {
      throw std::runtime_error(std::string("Tried to unify concrete types: ") +
                               TypeToStringConverter{}(type_a) + " and " +
                               TypeToStringConverter{}(type_b));
    }

    for (const auto &[parameter_type_a, parameter_type_b] :
         std::views::zip(type_a->m_argument_types, type_b->m_argument_types)) {
      unify(parameter_type_a, parameter_type_b);
    }

    unify(type_a->m_return_type, type_b->m_return_type);
  }

  void unify(const Type &type_a, const TypeVariable &type_b) {
    // Define it once in the other method
    unify(type_b, type_a);
  }

  void unify(const TypeVariable &type_a, const Type &type_b) {
    if (std::holds_alternative<TypeVariable>(type_b)) {
      unify(type_a, std::get<TypeVariable>(type_b));
      return;
    }

    if (std::holds_alternative<NeverType>(type_b)) {
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
      // This root has already been unified, satisfying all constraints
      return;
    }

    // We've not unified this root with a concrete type yet, does it have a type
    // class constraint?
    auto class_iter = m_resolved_type_class.find(root_a);
    if (class_iter != m_resolved_type_class.end()) {
      const auto &resolved_type_class = class_iter->second;
      if (!is_type_in_type_class(resolved_type_class, type_b)) {
        throw std::runtime_error("Could not resolve type class constraints "
                                 "from resolved type_class: " +
                                 resolved_type_class +
                                 " and concrete type: " + type_b);
      }
      // We satisfied the constraint, move to add new concrete type
    }

    // No entry
    m_resolved_type.emplace(root_a, type_b);
  }

  void constrain(const TypeVariable &type,
                 const PrimitiveTypeClass &type_class) {
    auto root = m_union_find.find(type.m_id);

    // Do we already have a concrete type for this var?
    auto iter = m_resolved_type.find(root);
    if (iter != m_resolved_type.end()) {
      const auto &resolved_type = iter->second;
      if (!is_type_in_type_class(type_class, resolved_type)) {
        throw std::runtime_error(
            "Could not resolve concrete type: " + resolved_type +
            " and type_class: " + type_class);
      }
      // All good, constraint satisfied
      return;
    }

    // Have we constrained this root before?
    auto class_iter = m_resolved_type_class.find(root);
    if (class_iter != m_resolved_type_class.end()) {
      auto &resolved_type_class = class_iter->second;
      if (!resolves(type_class, resolved_type_class) &&
          !resolves(resolved_type_class, type_class)) {
        // Cannot resolve one with the other? they are disjoint
        throw std::runtime_error("Could not resolve type class constraints "
                                 "from resolved type_class: " +
                                 resolved_type_class +
                                 " and type_class: " + type_class);
      }
      // Constraint satisfied, do we need to update it? Did we tighten the
      // constraint?
      if (!resolves(resolved_type_class, type_class)) {
        // type_class is more constraining or equal
        resolved_type_class = type_class;
      } else {
        // Original was more constraining
      }
      // All good, constraints satified
      return;
    }

    // Need to enforce that we end up with a type in this type class
    m_resolved_type_class.emplace(root, type_class);
  }

  void unify(const TypeVariable &type_a, const TypeVariable &type_b) {
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

    auto new_root = m_union_find.find(type_a.m_id);

    // Merge constraints
    auto class_iter_a = m_resolved_type_class.find(root_a);
    auto class_iter_b = m_resolved_type_class.find(root_b);
    if (class_iter_a != m_resolved_type_class.end() &&
        class_iter_b != m_resolved_type_class.end()) {
      const auto type_class_a = class_iter_a->second;
      const auto type_class_b = class_iter_b->second;
      if (!resolves(type_class_a, type_class_b) &&
          !resolves(type_class_b, type_class_a)) {
        // Can't resolve one with the other, they are disjoint
        throw std::runtime_error("Could not resolve type class constraints "
                                 "from type_class: " +
                                 type_class_a +
                                 " and type_class: " + type_class_b);
      }
      if (resolves(type_class_a, type_class_b)) {
        // Take type_class_a because it is more constraining, or they are the
        // same
        m_resolved_type_class[new_root] = type_class_a;
      } else {
        m_resolved_type_class[new_root] = type_class_b;
      }
    } else if (class_iter_a != m_resolved_type_class.end()) {
      const auto type_class_a = class_iter_a->second;
      m_resolved_type_class[new_root] = type_class_a;
    } else if (class_iter_b != m_resolved_type_class.end()) {
      const auto type_class_b = class_iter_b->second;
      m_resolved_type_class[new_root] = type_class_b;
    } else {
      // No type class constraints
    }

    auto merged_type_class_iter = m_resolved_type_class.find(new_root);
    if (merged_type_class_iter != m_resolved_type_class.end()) {
      const auto merged_type_class = merged_type_class_iter->second;
      // Need to validate the concrete type
      const auto &concrete_type =
          (iter_a != m_resolved_type.end()) ? iter_a->second : iter_b->second;
      if (iter_a != m_resolved_type.end() || iter_b != m_resolved_type.end()) {
        if (!is_type_in_type_class(merged_type_class, concrete_type)) {
          // We can't satisfy the merged constraint with this concrete class
          throw std::runtime_error("Could not resolve type class constraints "
                                   "from merged type_class: " +
                                   merged_type_class +
                                   " and concrete type: " + concrete_type);
        }
      }
    }

    if (iter_a != m_resolved_type.end() || iter_b != m_resolved_type.end()) {
      // Need to update the resolved types since someone was already concrete
      if (iter_a != m_resolved_type.end()) {
        // A is concrete, B is not
        const auto &concrete_a = iter_a->second;
        m_resolved_type.emplace(new_root, concrete_a);
      } else {
        const auto &concrete_b = iter_b->second;
        // B is concrete, A is not
        m_resolved_type.emplace(new_root, concrete_b);
      }
    }
  }

  Type find(const Type &type) {
    if (std::holds_alternative<TypeVariable>(type)) {
      return find(std::get<TypeVariable>(type));
    }
    return type;
  }

  Type find(const TypeVariable &type) {
    auto root = m_union_find.find(type.m_id);

    auto iter = m_resolved_type.find(root);
    if (iter != m_resolved_type.end()) {
      const auto &resolved_type = iter->second;
      return resolved_type;
    }

    // Not a concrete type yet
    return TypeVariable{{}, root};
  }

  UnionFind m_union_find{};
  std::unordered_map<size_t, Type> m_resolved_type;
  std::unordered_map<size_t, PrimitiveTypeClass> m_resolved_type_class;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
