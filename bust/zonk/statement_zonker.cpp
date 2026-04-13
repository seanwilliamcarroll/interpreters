//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement zonker implementation.
//*
//*
//****************************************************************************

#include "zonk/expression_zonker.hpp"
#include "zonk/let_binding_zonker.hpp"
#include <variant>
#include <zonk/statement_zonker.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

hir::Statement StatementZonker::zonk(hir::Statement statement) {
  return std::visit(*this, std::move(statement));
}

hir::Statement StatementZonker::operator()(hir::Expression expression) {
  return ExpressionZonker{m_ctx}.zonk(std::move(expression));
}

hir::Statement StatementZonker::operator()(hir::LetBinding let_binding) {
  return LetBindingZonker{m_ctx}.zonk(std::move(let_binding));
}

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
