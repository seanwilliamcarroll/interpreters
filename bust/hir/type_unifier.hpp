//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type unifier for HM-style type inference using union-find.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <exceptions.hpp>
#include <hir/type_registry.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>
#include <hir/union_find.hpp>
#include <types.hpp>

#include <optional>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <variant>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeUnifier {

  explicit TypeUnifier(TypeRegistry &type_registry)
      : m_type_registry(type_registry) {}

  // Adopt an existing UnifierState (e.g. carried forward on hir::Program
  // between passes) by moving its contents into this unifier's own fields.
  // The source state is left empty. Intended for the "aggregate-init an
  // empty unifier, then move state in" pattern at pass entry.
  void adopt_state(UnifierState state) {
    m_union_find = std::move(state.m_union_find);
    m_resolved_type_id = std::move(state.m_resolved_type_id);
    m_resolved_type_class = std::move(state.m_resolved_type_class);
  }

  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] std::string
  to_string(const auto &type) const {
    return m_type_registry.to_string(type);
  }

  TypeId
  new_type_var(std::optional<PrimitiveTypeClass> possible_constraint = {}) {
    auto new_id = m_union_find.add_node();
    auto new_type_var = TypeVariable{.m_id = new_id};
    auto new_type_id = m_type_registry.intern(new_type_var);

    if (!possible_constraint.has_value()) {
      return new_type_id;
    }
    // Check if the constraint conflicts, it shouldn't, but not bad to check
    const auto &constraint = possible_constraint.value();
    constrain(new_type_var, constraint);
    return new_type_id;
  }

  void unify(const TypeId &type_id_a, const TypeId &type_id_b) {
    const auto &type_a = m_type_registry.get(type_id_a);
    const auto &type_b = m_type_registry.get(type_id_b);
    if (type_id_a == m_type_registry.m_never ||
        type_id_b == m_type_registry.m_never) {
      return;
    }

    if (std::holds_alternative<TypeVariable>(type_a)) {
      unify(std::get<TypeVariable>(type_a), type_id_b);
      return;
    }

    if (std::holds_alternative<TypeVariable>(type_b)) {
      unify(type_id_a, std::get<TypeVariable>(type_b));
      return;
    }

    // We've been passed two concrete types

    if (type_a == type_b) {
      return;
    }

    // First check if they are both function types
    if (std::holds_alternative<FunctionType>(type_a) &&
        std::holds_alternative<FunctionType>(type_b)) {
      unify(std::get<FunctionType>(type_a), std::get<FunctionType>(type_b));
      return;
    }

    throw std::runtime_error("Tried to unify concrete types: " +
                             to_string(type_a) + " and " + to_string(type_b));
  }

  void unify(const FunctionType &type_a, const FunctionType &type_b) {
    if (type_a.m_parameters.size() != type_b.m_parameters.size()) {
      throw std::runtime_error(std::string("Tried to unify concrete types: ") +
                               to_string(type_a) + " and " + to_string(type_b));
    }

    for (const auto &[parameter_type_a, parameter_type_b] :
         std::views::zip(type_a.m_parameters, type_b.m_parameters)) {
      unify(parameter_type_a, parameter_type_b);
    }

    unify(type_a.m_return_type, type_b.m_return_type);
  }

  void unify(const TypeId &type_id_a, const TypeVariable &type_b) {
    // Define it once in the other method
    unify(type_b, type_id_a);
  }

  void unify(const TypeVariable &type_a, const TypeId &type_id_b) {
    const auto &type_b = m_type_registry.get(type_id_b);
    if (std::holds_alternative<TypeVariable>(type_b)) {
      unify(type_a, std::get<TypeVariable>(type_b));
      return;
    }

    if (type_id_b == m_type_registry.m_never) {
      return;
    }

    // We've been handled a concrete type, need to add as a resolved type

    auto root_a = m_union_find.find(type_a.m_id);

    auto iter_a = m_resolved_type_id.find(root_a);
    if (iter_a != m_resolved_type_id.end()) {
      const auto &concrete_a = iter_a->second;
      if (concrete_a != type_id_b) {
        throw std::runtime_error(
            "Could not resolve type variable " + to_string(type_a) + " (" +
            to_string(concrete_a) + ") and " + to_string(type_b));
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
                                 " and concrete type: " + to_string(type_b));
      }
      // We satisfied the constraint, move to add new concrete type
    }

    // No entry
    m_resolved_type_id.emplace(root_a, type_id_b);
  }

  void constrain(const TypeId &type_id, const PrimitiveTypeClass &type_class) {
    constrain(m_type_registry.as_type_variable(type_id), type_class);
  }

  void constrain(const TypeVariable &type,
                 const PrimitiveTypeClass &type_class) {
    auto root = m_union_find.find(type.m_id);

    // Do we already have a concrete type for this var?
    auto iter = m_resolved_type_id.find(root);
    if (iter != m_resolved_type_id.end()) {
      const auto &resolved_type_id = iter->second;
      const auto &resolved_type = m_type_registry.get(resolved_type_id);
      if (!is_type_in_type_class(type_class, resolved_type)) {
        throw std::runtime_error(
            "Could not resolve concrete type: " + to_string(resolved_type) +
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

    auto iter_a = m_resolved_type_id.find(root_a);
    auto iter_b = m_resolved_type_id.find(root_b);
    if (iter_a != m_resolved_type_id.end() &&
        iter_b != m_resolved_type_id.end()) {
      // Should be a formality but check just to be sure
      const auto &concrete_a = iter_a->second;
      const auto &concrete_b = iter_b->second;
      if (concrete_a != concrete_b) {
        throw std::runtime_error(
            "Could not resolve type variables " + to_string(type_a) + " (" +
            to_string(concrete_a) + ") and " + to_string(type_b) + "(" +
            to_string(concrete_b) + ")");
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
      if (iter_a != m_resolved_type_id.end() ||
          iter_b != m_resolved_type_id.end()) {
        const auto &concrete_type_id = (iter_a != m_resolved_type_id.end())
                                           ? iter_a->second
                                           : iter_b->second;
        const auto &concrete_type = m_type_registry.get(concrete_type_id);
        if (!is_type_in_type_class(merged_type_class, concrete_type)) {
          // We can't satisfy the merged constraint with this concrete class
          throw std::runtime_error("Could not resolve type class constraints "
                                   "from merged type_class: " +
                                   merged_type_class + " and concrete type: " +
                                   to_string(concrete_type));
        }
      }
    }

    if (iter_a != m_resolved_type_id.end() ||
        iter_b != m_resolved_type_id.end()) {
      // Need to update the resolved types since someone was already concrete
      if (iter_a != m_resolved_type_id.end()) {
        // A is concrete, B is not
        const auto &concrete_a = iter_a->second;
        m_resolved_type_id.emplace(new_root, concrete_a);
      } else {
        const auto &concrete_b = iter_b->second;
        // B is concrete, A is not
        m_resolved_type_id.emplace(new_root, concrete_b);
      }
    }
  }

  TypeId find(const TypeId &type_id) {
    const auto &type = m_type_registry.get(type_id);
    if (std::holds_alternative<TypeVariable>(type)) {
      return find(std::get<TypeVariable>(type));
    }
    return type_id;
  }

  TypeId find(const TypeVariable &type) {
    auto root = m_union_find.find(type.m_id);

    auto iter = m_resolved_type_id.find(root);
    if (iter != m_resolved_type_id.end()) {
      const auto &resolved_type = iter->second;
      return resolved_type;
    }

    // Not a concrete type yet
    return m_type_registry.intern(TypeVariable{root});
  }

  std::optional<PrimitiveTypeClass> find_type_class(const TypeId &type_id) {
    return find_type_class(m_type_registry.as_type_variable(type_id));
  }

  std::optional<PrimitiveTypeClass> find_type_class(const TypeVariable &type) {
    auto root_id = find(type);

    const auto &root = m_type_registry.get(root_id);

    if (!std::holds_alternative<TypeVariable>(root)) {
      throw core::InternalCompilerError(
          "Should not call this method on a resolved type variable");
    }

    const auto &root_type_var = std::get<TypeVariable>(root);

    auto iter = m_resolved_type_class.find(root_type_var.m_id);
    if (iter == m_resolved_type_class.end()) {
      return {};
    }
    // A constraint exists
    auto constraint = iter->second;
    return {constraint};
  }

  UnifierState extract_state() {
    return {.m_union_find = std::move(m_union_find),
            .m_resolved_type_id = std::move(m_resolved_type_id),
            .m_resolved_type_class = std::move(m_resolved_type_class)};
  }

  TypeRegistry &m_type_registry;
  UnionFind m_union_find;
  std::unordered_map<size_t, TypeId> m_resolved_type_id;
  std::unordered_map<size_t, PrimitiveTypeClass> m_resolved_type_class;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
