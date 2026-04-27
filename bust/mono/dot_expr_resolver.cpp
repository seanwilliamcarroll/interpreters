//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the DotExpr pre-resolver.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <hir/nodes.hpp>
#include <mono/dot_expr_resolver.hpp>

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

hir::TypeId DotExprResolver::resolve_to_type(
    const std::unique_ptr<hir::DotExpr> &dot_expr) {

  resolve(dot_expr->m_expression);

  auto new_type = m_ctx.rewrite_type(dot_expr->m_expression.m_type);

  if (!m_ctx.m_parent.m_type_arena.is_tuple(new_type)) {
    throw core::CompilerException(
        "Monomorpher",
        "Tuple dot operator requires that the "
        "expression be resolvable to a concrete tuple type, not: " +
            m_ctx.m_parent.m_type_arena.to_string(new_type),
        dot_expr->m_expression.m_location);
  }

  const auto &tuple_type = m_ctx.m_parent.m_type_arena.as_tuple(new_type);
  const auto arity = tuple_type.m_fields.size();

  if (dot_expr->m_tuple_index >= arity) {
    throw core::CompilerException(
        "Monomorpher",
        "Tuple dot operator requires that accessor be strictly less than "
        "tuple "
        "arity, arity: " +
            std::to_string(arity) +
            " vs. accessor index: " + std::to_string(dot_expr->m_tuple_index),
        dot_expr->m_expression.m_location);
  }

  return tuple_type.m_fields[dot_expr->m_tuple_index];
}

void DotExprResolver::resolve(const hir::Expression &expression) {
  if (const auto *dot_expr_ptr = std::get_if<std::unique_ptr<hir::DotExpr>>(
          &expression.m_expression)) {
    const auto &dot_expr = *dot_expr_ptr;
    // DotExpr's special case where we want to substitute the expression's
    // original type with the inner type that we now have access to
    m_ctx.m_substitution_mapping[expression.m_type] = resolve_to_type(dot_expr);
  } else {
    std::visit(*this, expression.m_expression);
  }
}

void DotExprResolver::resolve(const hir::Block &block) {
  for (const auto &statement : block.m_statements) {
    std::visit(
        [&](const auto &s) {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, hir::Expression>) {
            resolve(s);
          } else if constexpr (std::is_same_v<T, hir::LetBinding>) {
            resolve(s.m_expression);
          }
        },
        statement);
  }
  if (block.m_final_expression.has_value()) {
    resolve(*block.m_final_expression);
  }
}

// --- Leaf nodes: nothing to walk -------------------------------------------

void DotExprResolver::operator()(const hir::Identifier & /*unused*/) {}
void DotExprResolver::operator()(const hir::Unit & /*unused*/) {}
void DotExprResolver::operator()(const hir::I8 & /*unused*/) {}
void DotExprResolver::operator()(const hir::I32 & /*unused*/) {}
void DotExprResolver::operator()(const hir::I64 & /*unused*/) {}
void DotExprResolver::operator()(const hir::Bool & /*unused*/) {}
void DotExprResolver::operator()(const hir::Char & /*unused*/) {}

// --- Composite nodes: recurse into children --------------------------------

void DotExprResolver::operator()(
    const std::unique_ptr<hir::TupleExpr> &tuple_expr) {
  for (const auto &field : tuple_expr->m_fields) {
    resolve(field);
  }
}

void DotExprResolver::operator()(const std::unique_ptr<hir::Block> &block) {
  resolve(*block);
}

void DotExprResolver::operator()(const std::unique_ptr<hir::IfExpr> &if_expr) {
  resolve(if_expr->m_condition);
  resolve(if_expr->m_then_block);
  if (if_expr->m_else_block.has_value()) {
    resolve(*if_expr->m_else_block);
  }
}

void DotExprResolver::operator()(
    const std::unique_ptr<hir::CallExpr> &call_expr) {
  resolve(call_expr->m_callee);
  for (const auto &argument : call_expr->m_arguments) {
    resolve(argument);
  }
}

void DotExprResolver::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expr) {
  resolve(binary_expr->m_lhs);
  resolve(binary_expr->m_rhs);
}

void DotExprResolver::operator()(
    const std::unique_ptr<hir::UnaryExpr> &unary_expr) {
  resolve(unary_expr->m_expression);
}

void DotExprResolver::operator()(
    const std::unique_ptr<hir::ReturnExpr> &return_expr) {
  resolve(return_expr->m_expression);
}

void DotExprResolver::operator()(
    const std::unique_ptr<hir::CastExpr> &cast_expr) {
  resolve(cast_expr->m_expression);
}

void DotExprResolver::operator()(
    const std::unique_ptr<hir::LambdaExpr> &lambda_expr) {
  resolve(lambda_expr->m_body);
}

// --- The case you will fill in ---------------------------------------------
//
// For a DotExpr:
//   1. Walk the target expression first (it may contain nested DotExprs).
//   2. Rewrite the target's type through the current substitution mapping
//      (m_ctx.rewrite_type) to get its concrete form.
//   3. Verify the concrete type is a TupleType; throw otherwise.
//   4. Bounds-check `m_tuple_index` against the tuple's arity; throw if out
//      of range.
//   5. Unify the DotExpr's own result type (a fresh type variable allocated
//      by the type checker) with `tuple.m_fields[m_tuple_index]` via
//      `m_ctx.m_parent.m_type_unifier.unify(...)`. That edge is what lets
//      `rewrite_type` later return a concrete type for the outer expression.
//
void DotExprResolver::operator()(
    const std::unique_ptr<hir::DotExpr> & /*unused*/) {
  throw core::InternalCompilerError("Unreachable!!");
}

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
