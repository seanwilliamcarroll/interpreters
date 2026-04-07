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

#include "ast/nodes.hpp"
#include "ast/types.hpp"
#include "exceptions.hpp"
#include "hir/checker_context.hpp"
#include "hir/environment.hpp"
#include "hir/nodes.hpp"
#include "hir/top_item_checker.hpp"
#include "hir/type_converter.hpp"
#include "hir/type_unifier.hpp"
#include "hir/type_visitors.hpp"
#include "hir/types.hpp"
#include "operators.hpp"
#include "source_location.hpp"
#include "types.hpp"
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_checker.hpp>
#include <variant>

//****************************************************************************
namespace bust {
//****************************************************************************

hir::Program TypeChecker::operator()(const ast::Program &program) {
  auto checker_context = hir::CheckerContext{
      .m_env = m_env, .m_return_type_stack{}, .m_type_unifier{}};

  // First pass to collect function signatures
  for (const auto &top_item : program.m_items) {
    if (!std::holds_alternative<ast::FunctionDef>(top_item)) {
      continue;
    }
    const auto &function_def = std::get<ast::FunctionDef>(top_item);
    hir::TopItemChecker{checker_context}.collect_function_signature(
        function_def);
  }

  // Second pass to actually type check everything
  std::vector<hir::TopItem> typed_items;
  typed_items.reserve(program.m_items.size());
  for (const auto &top_item : program.m_items) {
    typed_items.push_back(
        std::visit(hir::TopItemChecker{checker_context}, top_item));
  }

  return {{program.m_location}, std::move(typed_items)};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
