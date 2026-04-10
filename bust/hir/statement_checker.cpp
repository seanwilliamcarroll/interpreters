//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement and block checker implementation.
//*
//*
//****************************************************************************

#include <hir/expression_checker.hpp>
#include <hir/let_binding_checker.hpp>
#include <hir/statement_checker.hpp>
#include <variant>

#include <ast/nodes.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

Statement StatementChecker::operator()(const ast::LetBinding &let_binding) {
  return LetBindingChecker{m_ctx}(let_binding);
}

Statement StatementChecker::operator()(const ast::Expression &expression) {
  return ExpressionChecker{m_ctx}.check_expression(expression);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
