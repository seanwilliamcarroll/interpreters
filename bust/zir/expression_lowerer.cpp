//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the expression lowerer.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <nodes.hpp>
#include <types.hpp>
#include <zir/arena.hpp>
#include <zir/context.hpp>
#include <zir/expression_lowerer.hpp>
#include <zir/nodes.hpp>
#include <zir/statement_lowerer.hpp>
#include <zir/types.hpp>

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
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
  return m_ctx.m_arena.push(std::move(new_expression));
}

IdentifierExpr ExpressionLowerer::lower(const hir::Identifier &identifier) {
  auto new_type = m_ctx.convert(identifier.m_type);

  auto binding = Binding{.m_name = identifier.m_name, .m_type = new_type};
  auto binding_id = m_ctx.m_arena.push(std::move(binding));

  return {.m_id = binding_id};
}

ExprKind ExpressionLowerer::operator()(const hir::Identifier &identifier) {
  return lower(identifier);
}

ExprKind ExpressionLowerer::operator()(const hir::LiteralUnit & /*unused*/) {
  return Unit{};
}

ExprKind ExpressionLowerer::operator()(const hir::LiteralI8 &literal) {
  return I8{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::LiteralI32 &literal) {
  return I32{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::LiteralI64 &literal) {
  return I64{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::LiteralBool &literal) {
  return Bool{literal.m_value};
}

ExprKind ExpressionLowerer::operator()(const hir::LiteralChar &literal) {
  return Char{literal.m_value};
}

Block ExpressionLowerer::lower(const hir::Block &block) {
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
                 std::back_inserter(arguments),
                 [&](const auto &argument) { return lower(argument); });

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
  std::vector<IdentifierExpr> parameters;
  parameters.reserve(lambda_expr->m_parameters.size());
  std::transform(lambda_expr->m_parameters.begin(),
                 lambda_expr->m_parameters.end(),
                 std::back_inserter(parameters),
                 [&](const auto &parameter) { return lower(parameter); });

  return LambdaExpr{
      .m_parameters = std::move(parameters),
      .m_body = lower(lambda_expr->m_body),
      .m_return_type = m_ctx.convert(lambda_expr->m_return_type),
  };
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
