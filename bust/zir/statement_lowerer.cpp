//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the statement lowerer.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <zir/context.hpp>
#include <zir/environment.hpp>
#include <zir/expression_lowerer.hpp>
#include <zir/nodes.hpp>
#include <zir/statement_lowerer.hpp>

#include <variant>

#include "exceptions.hpp"

//****************************************************************************
namespace bust::zir {
//****************************************************************************

Statement StatementLowerer::lower(const hir::Statement &statement) {
  return std::visit(*this, statement);
}

Statement StatementLowerer::operator()(const hir::Expression &expression) {
  auto expr_id = ExpressionLowerer{m_ctx}.lower(expression);

  return ExpressionStatement{.m_expression = expr_id};
}

Statement StatementLowerer::operator()(const hir::LetBinding &let_binding) {
  // Potentially shadowing, so do a definition lowering
  auto identifier_expr =
      ExpressionLowerer{m_ctx}.lower_definition(let_binding.m_variable);

  auto binding_id = identifier_expr.m_id;

  m_ctx.env().define(let_binding.m_variable.m_name, binding_id);

  auto expr_id = ExpressionLowerer{m_ctx}.lower(let_binding.m_expression);

  return LetBinding{
      .m_identifier = binding_id,
      .m_expression = expr_id,
  };
}

Place StatementLowerer::lower(const hir::Place &place) {
  return std::visit(
      [&](const auto &s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, hir::Identifier>) {
          return ExpressionLowerer{m_ctx}.lower(s);
        }
      },
      place);
}

Statement StatementLowerer::operator()(const hir::Assignment &assignment) {
  return Assignment{
      .m_place = lower(assignment.m_place),
      .m_expression = ExpressionLowerer{m_ctx}.lower(assignment.m_expression),
  };
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
