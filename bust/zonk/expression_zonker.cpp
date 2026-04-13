//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression zonker implementation.
//*
//*
//****************************************************************************

#include "hir/nodes.hpp"
#include "zonk/statement_zonker.hpp"
#include <memory>
#include <optional>
#include <variant>
#include <zonk/expression_zonker.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

hir::Expression ExpressionZonker::zonk(hir::Expression expression) {
  // TODO: Resolve expression.m_type, then visit the inner ExprKind
  auto zonked_kind = std::visit(*this, std::move(expression.m_expression));

  auto new_type_id = m_ctx.find_and_register(expression.m_type);

  return {{expression.m_location}, new_type_id, std::move(zonked_kind)};
}

hir::ExprKind ExpressionZonker::zonk(hir::Block block) {
  return std::make_unique<hir::Block>(zonk_block(std::move(block)));
}

hir::Identifier ExpressionZonker::zonk(hir::Identifier identifier) {
  auto new_type_id = m_ctx.find_and_register(identifier.m_type);
  return {{identifier.m_location}, std::move(identifier.m_name), new_type_id};
}

hir::ExprKind ExpressionZonker::operator()(hir::Identifier identifier) {
  return zonk(std::move(identifier));
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralUnit literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralI8 literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralI32 literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralI64 literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralBool literal) {
  return literal;
}

hir::ExprKind ExpressionZonker::operator()(hir::LiteralChar literal) {
  return literal;
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::Block> block_ptr) {
  auto &block = *block_ptr;
  return zonk(std::move(block));
}

hir::Block ExpressionZonker::zonk_block(hir::Block block) {
  std::vector<hir::Statement> zonked_statements;
  zonked_statements.reserve(block.m_statements.size());
  for (auto &statement : block.m_statements) {
    auto zonked_statement = StatementZonker{m_ctx}.zonk(std::move(statement));
    zonked_statements.push_back(std::move(zonked_statement));
  }

  auto zonked_final_expression =
      block.m_final_expression.has_value()
          ? std::optional<hir::Expression>(
                zonk(std::move(block.m_final_expression.value())))
          : std::nullopt;

  auto new_type_id = m_ctx.find_and_register(block.m_type);

  return {{block.m_location},
          new_type_id,
          std::move(zonked_statements),
          std::move(zonked_final_expression)};
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::IfExpr> if_expr) {
  auto zonked_condition = zonk(std::move(if_expr->m_condition));

  auto zonked_then_block = zonk_block(std::move(if_expr->m_then_block));

  auto zonked_else_block = if_expr->m_else_block.has_value()
                               ? std::optional<hir::Block>(zonk_block(
                                     std::move(if_expr->m_else_block.value())))
                               : std::nullopt;

  return std::make_unique<hir::IfExpr>(
      hir::IfExpr{std::move(zonked_condition), std::move(zonked_then_block),
                  std::move(zonked_else_block)});
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::CallExpr> call_expr) {
  auto zonked_callee = zonk(std::move(call_expr->m_callee));

  std::vector<hir::Expression> zonked_arguments;
  zonked_arguments.reserve(call_expr->m_arguments.size());
  for (auto &argument : call_expr->m_arguments) {
    zonked_arguments.push_back(zonk(std::move(argument)));
  }

  return std::make_unique<hir::CallExpr>(
      hir::CallExpr{std::move(zonked_callee), std::move(zonked_arguments)});
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::BinaryExpr> binary_expr) {
  auto zonked_lhs = zonk(std::move(binary_expr->m_lhs));
  auto zonked_rhs = zonk(std::move(binary_expr->m_rhs));

  return std::make_unique<hir::BinaryExpr>(hir::BinaryExpr{
      binary_expr->m_operator, std::move(zonked_lhs), std::move(zonked_rhs)});
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::UnaryExpr> unary_expr) {
  auto zonked_expression = zonk(std::move(unary_expr->m_expression));

  return std::make_unique<hir::UnaryExpr>(
      hir::UnaryExpr{unary_expr->m_operator, std::move(zonked_expression)});
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::ReturnExpr> return_expr) {
  auto zonked_expression = zonk(std::move(return_expr->m_expression));

  return std::make_unique<hir::ReturnExpr>(
      hir::ReturnExpr{std::move(zonked_expression)});
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::CastExpr> cast_expr) {
  auto zonked_expression = zonk(std::move(cast_expr->m_expression));

  auto zonked_new_type_id = m_ctx.find_and_register(cast_expr->m_new_type);

  return std::make_unique<hir::CastExpr>(
      hir::CastExpr{std::move(zonked_expression), zonked_new_type_id});
}

hir::ExprKind
ExpressionZonker::operator()(std::unique_ptr<hir::LambdaExpr> lambda_expr) {
  std::vector<hir::Identifier> zonked_parameters;
  zonked_parameters.reserve(lambda_expr->m_parameters.size());
  for (auto &parameter : lambda_expr->m_parameters) {
    zonked_parameters.push_back(zonk(std::move(parameter)));
  }

  auto zonked_body = zonk_block(std::move(lambda_expr->m_body));

  auto zonked_return_type_id =
      m_ctx.find_and_register(lambda_expr->m_return_type);

  return std::make_unique<hir::LambdaExpr>(
      hir::LambdaExpr{std::move(zonked_parameters), std::move(zonked_body),
                      zonked_return_type_id});
}

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
