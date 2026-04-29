//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the expression lowerer.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <hir/nodes.hpp>
#include <nodes.hpp>
#include <types.hpp>
#include <zir/arena.hpp>
#include <zir/context.hpp>
#include <zir/environment.hpp>
#include <zir/expression_lowerer.hpp>
#include <zir/free_variable_collector.hpp>
#include <zir/nodes.hpp>
#include <zir/statement_lowerer.hpp>

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

ExprId ExpressionLowerer::lower(const hir::Expression &expression) {
  // Resolve the type
  auto new_type = m_ctx.convert(expression.m_type);

  auto expr_kind = std::visit(*this, expression.m_expression);

  auto new_expression =
      Expression{.m_type_id = new_type, .m_expr_kind = expr_kind};

  // TODO: Location
  // One place we actually push expressions
  return m_ctx.arena().push(std::move(new_expression));
}

IdentifierExpr ExpressionLowerer::lower(const hir::Identifier &identifier) {
  auto maybe_id = m_ctx.env().lookup(identifier.m_name);
  if (maybe_id.has_value()) {
    return {.m_id = maybe_id.value()};
  }
  throw core::InternalCompilerError(
      "This binding should have been lowered through lower_definition! "
      "Identifier: " +
      identifier.m_name + " of type: " + m_ctx.to_string(identifier.m_type));
}

IdentifierExpr
ExpressionLowerer::lower_definition(const hir::Identifier &identifier) {
  // Always create a fresh BindingId when defining
  auto new_type = m_ctx.convert(identifier.m_type);

  auto binding = Binding{.m_name = identifier.m_name, .m_type = new_type};
  auto binding_id = m_ctx.arena().push(std::move(binding));

  m_ctx.env().define(identifier.m_name, binding_id);

  return {.m_id = binding_id};
}

ExprKind ExpressionLowerer::operator()(
    const std::unique_ptr<hir::TupleExpr> &tuple_expr) {

  std::vector<ExprId> fields;
  fields.reserve(tuple_expr->m_fields.size());
  std::transform(tuple_expr->m_fields.cbegin(), tuple_expr->m_fields.cend(),
                 std::back_inserter(fields),
                 [&](const auto &field) -> ExprId { return lower(field); });

  return TupleExpr{
      .m_fields = std::move(fields),
  };
}

ExprKind ExpressionLowerer::operator()(const hir::Identifier &identifier) {
  return lower(identifier);
}

ExprKind ExpressionLowerer::operator()(const hir::Unit & /*unused*/) {
  return Unit{};
}

ExprKind ExpressionLowerer::operator()(const hir::I8 &literal) {
  return I8{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::I32 &literal) {
  return I32{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::I64 &literal) {
  return I64{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::Bool &literal) {
  return Bool{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::Char &literal) {
  return Char{literal.m_value};
}

Block ExpressionLowerer::lower(const hir::Block &block) {
  ScopeGuard guard{m_ctx.env()};
  std::vector<Statement> new_statements;
  new_statements.reserve(block.m_statements.size());
  for (const auto &statement : block.m_statements) {
    new_statements.emplace_back(StatementLowerer{m_ctx}.lower(statement));
  }

  auto final_expression =
      block.m_final_expression.and_then([&](const auto &expression) {
        return std::make_optional(lower(expression));
      });

  return {.m_statements = std::move(new_statements),
          .m_final_expression = final_expression};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::Block> &block) {
  return lower(*block);
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::IfExpr> &if_expr) {
  return IfExpr{.m_condition = lower(if_expr->m_condition),
                .m_then_block = lower(if_expr->m_then_block),
                .m_else_block =
                    if_expr->m_else_block.and_then([&](const auto &expression) {
                      return std::make_optional(lower(expression));
                    })};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::CallExpr> &call_expr) {
  std::vector<ExprId> arguments;
  arguments.reserve(call_expr->m_arguments.size());
  std::transform(call_expr->m_arguments.begin(), call_expr->m_arguments.end(),
                 std::back_inserter(arguments), [&](const auto &argument) {
                   // Check if we're calling a globally bound function
                   return lower(argument);
                 });

  return CallExpr{.m_callee = lower(call_expr->m_callee),
                  .m_arguments = std::move(arguments)};
}

ExprKind ExpressionLowerer::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expr) {
  return BinaryExpr{
      .m_operator = binary_expr->m_operator,
      .m_lhs = lower(binary_expr->m_lhs),
      .m_rhs = lower(binary_expr->m_rhs),
  };
}

ExprKind ExpressionLowerer::operator()(
    const std::unique_ptr<hir::UnaryExpr> &unary_expr) {
  return UnaryExpr{
      .m_operator = unary_expr->m_operator,
      .m_expression = lower(unary_expr->m_expression),
  };
}

ExprKind ExpressionLowerer::operator()(
    const std::unique_ptr<hir::ReturnExpr> &return_expr) {
  return ReturnExpr{.m_expression = lower(return_expr->m_expression)};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::CastExpr> &cast_expr) {
  return CastExpr{
      .m_expression = lower(cast_expr->m_expression),
      .m_new_type = m_ctx.convert(cast_expr->m_new_type),
  };
}

ExprKind ExpressionLowerer::operator()(
    const std::unique_ptr<hir::LambdaExpr> &lambda_expr) {
  ScopeGuard guard{m_ctx.env()};

  std::vector<IdentifierExpr> known_bindings;
  std::vector<IdentifierExpr> parameters;
  parameters.reserve(lambda_expr->m_parameters.size());
  for (const auto &parameter : lambda_expr->m_parameters) {
    auto new_identifier = lower_definition(parameter);
    m_ctx.env().define(parameter.m_name, new_identifier.m_id);
    parameters.emplace_back(new_identifier);
    known_bindings.emplace_back(new_identifier);
  }

  for (const auto &[_, global_id] : m_ctx.global_bindings()) {
    known_bindings.emplace_back(IdentifierExpr{.m_id = global_id});
  }

  auto body = lower(lambda_expr->m_body);

  auto capture_set = FreeVariableCollector(m_ctx, known_bindings).collect(body);
  auto captures =
      std::vector<IdentifierExpr>(std::make_move_iterator(capture_set.begin()),
                                  std::make_move_iterator(capture_set.end()));
  capture_set.clear();

  return LambdaExpr{
      .m_parameters = std::move(parameters),
      .m_body = body,
      .m_return_type = m_ctx.convert(lambda_expr->m_return_type),
      .m_captures = captures,
  };
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::DotExpr> &dot_expr) {
  auto expression = lower(dot_expr->m_expression);

  return DotExpr{
      .m_expression = expression,
      .m_tuple_index = dot_expr->m_tuple_index,
  };
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
