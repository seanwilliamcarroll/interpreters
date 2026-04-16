//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the expression lowerer.
//*
//*
//****************************************************************************

#include "hir/nodes.hpp"
#include "zir/nodes.hpp"
#include "zir/statement_lowerer.hpp"
#include <optional>
#include <zir/expression_lowerer.hpp>

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

ExprKind ExpressionLowerer::operator()(const hir::LiteralUnit &) {
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
      block.m_final_expression.and_then([&](const hir::Expression &expression) {
        return std::optional<ExprId>(lower(expression));
      });

  return {.m_statements = std::move(new_statements),
          .m_final_expression = final_expression};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::Block> &block) {
  return lower(*block);
}

ExprKind ExpressionLowerer::operator()(const std::unique_ptr<hir::IfExpr> &) {
  return {};
}

ExprKind ExpressionLowerer::operator()(const std::unique_ptr<hir::CallExpr> &) {
  return {};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::BinaryExpr> &) {
  return {};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::UnaryExpr> &) {
  return {};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::ReturnExpr> &) {
  return {};
}

ExprKind ExpressionLowerer::operator()(const std::unique_ptr<hir::CastExpr> &) {
  return {};
}

ExprKind
ExpressionLowerer::operator()(const std::unique_ptr<hir::LambdaExpr> &) {
  return {};
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
