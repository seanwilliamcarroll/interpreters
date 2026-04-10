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

Type BlockChecker::get_statement_type(const Statement &statement) {
  if (std::holds_alternative<Expression>(statement)) {
    return std::get<Expression>(statement).m_type;
  }
  const auto &let_binding = std::get<LetBinding>(statement);
  return PrimitiveTypeValue{{let_binding.m_location}, PrimitiveType::UNIT};
}

Block BlockChecker::check_block(const ast::Block &block) {
  m_ctx.m_env.push_scope();

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
              : !statements.empty()
                  ? get_statement_type(statements.back())
                  : PrimitiveTypeValue{{block.m_location}, PrimitiveType::UNIT};

  m_ctx.m_env.pop_scope();
  return {{block.m_location},
          std::move(type),
          std::move(statements),
          std::move(final_expression)};
}

Block BlockChecker::check_block_with_parameters(
    const std::vector<Identifier> &parameters, const ast::Block &ast_block) {
  m_ctx.m_env.push_scope();
  for (const auto &parameter : parameters) {
    m_ctx.m_env.define(parameter.m_name, parameter.m_type);
  }
  auto block = check_block(ast_block);
  m_ctx.m_env.pop_scope();

  return block;
}

Block BlockChecker::check_callable_body(
    const std::vector<Identifier> &parameters, const Type &return_type,
    const ast::Block &ast_body) {
  m_ctx.m_return_type_stack.push_back(return_type);
  auto body = check_block_with_parameters(parameters, ast_body);
  m_ctx.m_return_type_stack.pop_back();
  return body;
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
