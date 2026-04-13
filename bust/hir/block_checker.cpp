//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Block checker implementation.
//*
//*
//****************************************************************************

#include <hir/block_checker.hpp>
#include <hir/environment.hpp>
#include <hir/expression_checker.hpp>
#include <hir/statement_checker.hpp>
#include <optional>
#include <source_location.hpp>
#include <types.hpp>
#include <utility>
#include <variant>

#include <ast/nodes.hpp>
#include <hir/context.hpp>
#include <hir/nodes.hpp>
#include <hir/types.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

TypeId BlockChecker::get_statement_type(const Statement &statement) {
  if (std::holds_alternative<Expression>(statement)) {
    return std::get<Expression>(statement).m_type;
  }
  return m_ctx.m_type_registry.m_unit;
}

Block BlockChecker::check_block(const ast::Block &block) {
  ScopeGuard guard(m_ctx.m_env);

  std::vector<Statement> statements;
  statements.reserve(block.m_statements.size());
  for (const auto &statement : block.m_statements) {
    statements.emplace_back(std::visit(StatementChecker{m_ctx}, (statement)));
  }

  auto final_expression =
      block.m_final_expression.has_value()
          ? std::optional<Expression>(ExpressionChecker{m_ctx}.check_expression(
                block.m_final_expression.value()))
          : std::nullopt;

  auto type = final_expression.has_value() ? final_expression.value().m_type
              : !statements.empty() ? get_statement_type(statements.back())
                                    : m_ctx.m_type_registry.m_unit;

  return {{block.m_location},
          type,
          std::move(statements),
          std::move(final_expression)};
}

Block BlockChecker::check_block_with_parameters(
    const std::vector<Identifier> &parameters, const ast::Block &ast_block) {
  ScopeGuard guard(m_ctx.m_env);
  for (const auto &parameter : parameters) {
    m_ctx.m_env.define(parameter.m_name, parameter.m_type);
  }
  return check_block(ast_block);
}

Block BlockChecker::check_callable_body(
    const std::vector<Identifier> &parameters, const TypeId &return_type,
    const ast::Block &ast_body) {
  m_ctx.m_return_type_stack.push_back(return_type);
  auto body = check_block_with_parameters(parameters, ast_body);
  m_ctx.m_return_type_stack.pop_back();
  return body;
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
