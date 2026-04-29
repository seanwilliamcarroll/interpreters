//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the expression substituter visitor.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/nodes.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <mono/context.hpp>
#include <mono/expression_substituter.hpp>
#include <mono/let_binding_monomorpher.hpp>
#include <mono/specialization.hpp>
#include <nodes.hpp>
#include <source_location.hpp>
#include <types.hpp>

#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

hir::Expression
ExpressionSubstituter::substitute(const hir::Expression &expression) {
  // Update the type, visit the exprkind
  auto new_type = m_ctx.rewrite_type(expression.m_type);

  auto substituted_expr_kind = std::visit(*this, expression.m_expression);

  // Need to handle the special dot expr case where we pull the type out of the
  // tuple
  if (auto *dot_expr =
          std::get_if<std::unique_ptr<hir::DotExpr>>(&substituted_expr_kind)) {
    const auto &tuple_type =
        m_ctx.m_parent.m_type_arena.as_tuple((*dot_expr)->m_expression.m_type);
    new_type =
        m_ctx.rewrite_type(tuple_type.m_fields[(*dot_expr)->m_tuple_index]);
  }

  return {
      {expression.m_location},
      new_type,
      std::move(substituted_expr_kind),
  };
}

hir::Identifier
ExpressionSubstituter::substitute(const hir::Identifier &identifier) {
  auto new_type = m_ctx.rewrite_type(identifier.m_type);

  const auto *possible_specialization =
      m_ctx.m_parent.m_env.lookup(identifier.m_id, new_type);

  if (possible_specialization == nullptr) {
    return hir::Identifier{
        {identifier.m_location},
        identifier.m_name,
        identifier.m_id,
        new_type,
    };
  }
  const auto &specialization = *possible_specialization;

  return hir::Identifier{{identifier.m_location},
                         specialization.m_mangled_name,
                         specialization.m_new_id,
                         new_type};
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::TupleExpr> &tuple_expr) {

  std::vector<hir::Expression> fields;
  fields.reserve(tuple_expr->m_fields.size());
  for (const auto &field : tuple_expr->m_fields) {
    fields.emplace_back(substitute(field));
  }

  return std::make_unique<hir::TupleExpr>(hir::TupleExpr{
      .m_fields = std::move(fields),
  });
}

hir::ExprKind
ExpressionSubstituter::operator()(const hir::Identifier &identifier) {
  return substitute(identifier);
}

hir::ExprKind ExpressionSubstituter::operator()(const hir::Unit &literal) {
  return literal;
}

hir::ExprKind ExpressionSubstituter::operator()(const hir::I8 &literal) {
  return literal;
}

hir::ExprKind ExpressionSubstituter::operator()(const hir::I32 &literal) {
  return literal;
}

hir::ExprKind ExpressionSubstituter::operator()(const hir::I64 &literal) {
  return literal;
}

hir::ExprKind ExpressionSubstituter::operator()(const hir::Bool &literal) {
  return literal;
}

hir::ExprKind ExpressionSubstituter::operator()(const hir::Char &literal) {
  return literal;
}

hir::Block ExpressionSubstituter::substitute(const hir::Block &block) {
  ScopeGuard guard{m_ctx.m_parent.m_env};

  // Here is where the possibility of new let bindings comes up
  auto new_type = m_ctx.rewrite_type(block.m_type);

  std::vector<hir::Statement> new_statements;
  for (const auto &statement : block.m_statements) {
    std::visit(
        [&](const auto &s) {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, hir::Expression>) {
            new_statements.emplace_back(substitute(s));
          } else if constexpr (std::is_same_v<T, hir::LetBinding>) {
            auto new_let_bindings =
                LetBindingMonomorpher{m_ctx.m_parent}.monomorph(
                    s, m_ctx.substitution_mapping());
            new_statements.insert(
                new_statements.end(),
                std::make_move_iterator(new_let_bindings.begin()),
                std::make_move_iterator(new_let_bindings.end()));
            new_let_bindings.clear();
          }
        },
        statement);
  }

  auto final_expression =
      block.m_final_expression.and_then([&](const auto &final_expression) {
        return std::make_optional(substitute(final_expression));
      });

  return {{block.m_location},
          new_type,
          std::move(new_statements),
          std::move(final_expression)};
}

hir::ExprKind
ExpressionSubstituter::operator()(const std::unique_ptr<hir::Block> &block) {
  return std::make_unique<hir::Block>(substitute(*block));
}

hir::ExprKind
ExpressionSubstituter::operator()(const std::unique_ptr<hir::IfExpr> &if_expr) {

  auto condition = substitute(if_expr->m_condition);
  auto then_block = substitute(if_expr->m_then_block);
  auto else_block = if_expr->m_else_block.and_then([&](const auto &else_block) {
    return std::make_optional(substitute(else_block));
  });

  return std::make_unique<hir::IfExpr>(hir::IfExpr{
      std::move(condition), std::move(then_block), std::move(else_block)});
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::CallExpr> &call_expr) {

  auto callee = substitute(call_expr->m_callee);

  std::vector<hir::Expression> arguments;
  arguments.reserve(call_expr->m_arguments.size());
  for (const auto &argument : call_expr->m_arguments) {
    arguments.emplace_back(substitute(argument));
  }

  return std::make_unique<hir::CallExpr>(
      hir::CallExpr{std::move(callee), std::move(arguments)});
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expr) {

  auto lhs = substitute(binary_expr->m_lhs);
  auto rhs = substitute(binary_expr->m_rhs);

  return std::make_unique<hir::BinaryExpr>(
      hir::BinaryExpr{binary_expr->m_operator, std::move(lhs), std::move(rhs)});
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::UnaryExpr> &unary_expr) {

  auto expr = substitute(unary_expr->m_expression);

  return std::make_unique<hir::UnaryExpr>(
      hir::UnaryExpr{unary_expr->m_operator, std::move(expr)});
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::ReturnExpr> &return_expr) {
  auto expr = substitute(return_expr->m_expression);
  return std::make_unique<hir::ReturnExpr>(hir::ReturnExpr{std::move(expr)});
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::CastExpr> &cast_expr) {
  auto expr = substitute(cast_expr->m_expression);
  auto new_type = m_ctx.rewrite_type(cast_expr->m_new_type);
  return std::make_unique<hir::CastExpr>(
      hir::CastExpr{std::move(expr), new_type});
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::LambdaExpr> &lambda_expr) {
  // Param BindingIds are preserved as-is across specializations. Each
  // specialization becomes its own emitted function, so the "two sites share
  // an id" concern is inert after this pass — nothing downstream does a
  // global BindingId lookup. If we ever add a pass post-mono that does, mint
  // fresh ids here and remap use-sites in the body.

  std::vector<hir::Identifier> new_parameters;
  new_parameters.reserve(lambda_expr->m_parameters.size());
  for (const auto &parameter : lambda_expr->m_parameters) {
    new_parameters.emplace_back(substitute(parameter));
  }

  auto new_return_type = m_ctx.rewrite_type(lambda_expr->m_return_type);

  auto new_body = substitute(lambda_expr->m_body);

  return std::make_unique<hir::LambdaExpr>(hir::LambdaExpr{
      std::move(new_parameters), std::move(new_body), new_return_type});
}

hir::ExprKind ExpressionSubstituter::operator()(
    const std::unique_ptr<hir::DotExpr> &dot_expr) {

  auto expression = substitute(dot_expr->m_expression);

  if (!m_ctx.m_parent.m_type_arena.is_tuple(expression.m_type)) {
    throw core::CompilerException(
        "Monomorpher",
        "Tuple dot operator requires that the "
        "expression be resolvable to a concrete tuple type, not: " +
            m_ctx.m_parent.m_type_arena.to_string(expression.m_type),
        expression.m_location);
  }

  const auto &tuple_type =
      m_ctx.m_parent.m_type_arena.as_tuple(expression.m_type);
  const auto arity = tuple_type.m_fields.size();

  if (dot_expr->m_tuple_index >= arity) {
    throw core::CompilerException(
        "TypeChecker",
        "Tuple dot operator requires that accessor be strictly less than tuple "
        "arity, arity: " +
            std::to_string(arity) +
            " vs. accessor index: " + std::to_string(dot_expr->m_tuple_index),
        expression.m_location);
  }

  return std::make_unique<hir::DotExpr>(hir::DotExpr{
      .m_expression = std::move(expression),
      .m_tuple_index = dot_expr->m_tuple_index,
  });
}

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
