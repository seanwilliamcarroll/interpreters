//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Block checker implementation.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <hir/block_checker.hpp>
#include <hir/context.hpp>
#include <hir/environment.hpp>
#include <hir/expression_checker.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/nodes.hpp>
#include <hir/statement_checker.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <source_location.hpp>

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

TypeId BlockChecker::get_statement_type(const Statement &statement) {
  if (std::holds_alternative<Expression>(statement)) {
    return std::get<Expression>(statement).m_type;
  }
  return m_ctx.type_arena().m_unit;
}

TypeId BlockChecker::get_inner_expression_type(const Statement &statement) {
  return std::visit(
      [](const auto &s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetBinding> ||
                      std::is_same_v<T, Assignment>) {
          return s.m_expression.m_type;
        } else if constexpr (std::is_same_v<T, Expression>) {
          return s.m_type;
        }
      },
      statement);
}

Block BlockChecker::check_block(const ast::Block &block) {
  ScopeGuard guard(m_ctx.env());

  bool has_diverging_statement = false;

  std::vector<Statement> statements;
  statements.reserve(block.m_statements.size());
  for (const auto &statement : block.m_statements) {
    statements.emplace_back(std::visit(StatementChecker{m_ctx}, (statement)));
    if (get_inner_expression_type(statements.back()) ==
        m_ctx.type_arena().m_never) {
      has_diverging_statement = true;
    }
  }

  auto final_expression =
      block.m_final_expression.and_then([&](const auto &expression) {
        return std::make_optional(
            ExpressionChecker{m_ctx}.check_expression(expression));
      });

  // No final expression and no diverging statement, block is unit type
  auto type = m_ctx.type_arena().m_unit;

  if (has_diverging_statement) {
    // Only if no final expression do we set diverging type
    type = m_ctx.type_arena().m_never;
  } else if (final_expression.has_value()) {
    type = final_expression.value().m_type;
  }

  return {
      .m_location = block.m_location,
      .m_type = type,
      .m_statements = std::move(statements),
      .m_final_expression = std::move(final_expression),
  };
}

Block BlockChecker::check_block_with_parameters(
    const std::vector<Parameter> &parameters, const ast::Block &ast_block) {
  ScopeGuard guard(m_ctx.env());
  for (const auto &parameter : parameters) {
    m_ctx.env().define(parameter.m_id.m_name, parameter.m_id.m_id,
                       parameter.m_id.m_type, parameter.m_is_mutable);
  }
  return check_block(ast_block);
}

Block BlockChecker::check_callable_body(
    const std::vector<Parameter> &parameters, const TypeId &return_type,
    const ast::Block &ast_body) {
  m_ctx.push_return_type(return_type);
  auto body = check_block_with_parameters(parameters, ast_body);
  m_ctx.pop_return_type();
  return body;
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
