//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the statement lowerer.
//*
//*
//****************************************************************************

#include "zir/expression_lowerer.hpp"
#include "zir/let_binding_lowerer.hpp"
#include <variant>
#include <zir/statement_lowerer.hpp>

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
  return LetBindingLowerer{m_ctx}.lower(let_binding);
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
