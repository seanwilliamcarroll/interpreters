//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of the TypeChecker pass.
//*
//*
//****************************************************************************

#include <type_checker.hpp>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "ast/nodes.hpp"
#include "hir/context.hpp"
#include "hir/nodes.hpp"
#include "hir/top_item_checker.hpp"
#include "hir/types.hpp"
#include "source_location.hpp"

//****************************************************************************
namespace bust {
//****************************************************************************

hir::Program TypeChecker::operator()(const ast::Program &program) {
  auto context =
      hir::Context{.m_env = m_env, .m_return_type_stack{}, .m_type_unifier{}};

  // First pass to collect function signatures
  for (const auto &top_item : program.m_items) {
    if (!std::holds_alternative<ast::FunctionDef>(top_item)) {
      continue;
    }
    const auto &function_def = std::get<ast::FunctionDef>(top_item);
    hir::TopItemChecker{context}.collect_function_signature(function_def);
  }

  // Second pass to actually type check everything
  std::vector<hir::TopItem> typed_items;
  typed_items.reserve(program.m_items.size());
  for (const auto &top_item : program.m_items) {
    typed_items.push_back(std::visit(hir::TopItemChecker{context}, top_item));
  }

  return {{program.m_location}, std::move(typed_items)};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
