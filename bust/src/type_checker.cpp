//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the TypeChecker pass.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <hir/context.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/nodes.hpp>
#include <hir/top_item_checker.hpp>
#include <hir/type_arena.hpp>
#include <hir/unifier_state.hpp>
#include <source_location.hpp>
#include <type_checker.hpp>

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************

hir::Program TypeChecker::operator()(const ast::Program &program) {
  auto context = hir::Context{};

  // First pass to collect function signatures
  for (const auto &top_item : program.m_items) {
    std::visit(
        [&](const auto &ti) {
          using T = std::decay_t<decltype(ti)>;
          if constexpr (std::is_same_v<T, ast::FunctionDef> ||
                        std::is_same_v<T, ast::ExternFunctionDeclaration>) {
            hir::TopItemChecker{context}.collect_function_signature(
                ti.m_signature);
          } else if constexpr (std::is_same_v<T, ast::LetBinding>) {
            // pass, leave as branch rather than else in case we add more top
            // items
          }
        },
        top_item);
  }

  // Second pass to actually type check everything
  std::vector<hir::TopItem> typed_items;
  typed_items.reserve(program.m_items.size());
  for (const auto &top_item : program.m_items) {
    typed_items.push_back(std::visit(hir::TopItemChecker{context}, top_item));
  }

  auto post_check_state = context.get_post_check_data();

  return {{program.m_location},
          std::move(post_check_state.m_type_arena),
          std::move(typed_items),
          std::move(post_check_state.m_unifier_state),
          std::move(post_check_state.m_instantiation_records),
          post_check_state.m_next_let_binding_id};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
