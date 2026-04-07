//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Statement and block checker implementation.
//*
//*
//****************************************************************************

#include "hir/statement_checker.hpp"

#include <variant>

#include "hir/expression_checker.hpp"
#include "hir/let_binding_checker.hpp"

//****************************************************************************
namespace bust::hir {
//****************************************************************************

Statement StatementChecker::operator()(const ast::LetBinding &let_binding) {
  return LetBindingChecker{m_ctx}(let_binding);
}

Statement StatementChecker::operator()(const ast::Expression &expression) {
  return std::visit(ExpressionChecker{m_ctx}, expression);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
