//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the let binding lowerer.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <zir/expression_lowerer.hpp>
#include <zir/let_binding_lowerer.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

LetBinding LetBindingLowerer::lower(const hir::LetBinding &let_binding) {

  auto identifier_expr = ExpressionLowerer{m_ctx}.lower(let_binding.m_variable);

  auto binding_id = identifier_expr.m_id;

  auto expr_id = ExpressionLowerer{m_ctx}.lower(let_binding.m_expression);

  return {.m_identifier = binding_id, .m_expression = expr_id};
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
