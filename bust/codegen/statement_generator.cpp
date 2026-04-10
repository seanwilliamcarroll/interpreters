//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of statement generator.
//*
//*
//****************************************************************************

#include <codegen/expression_generator.hpp>
#include <codegen/let_binding_generator.hpp>
#include <codegen/statement_generator.hpp>

#include <codegen/handle.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Handle StatementGenerator::operator()(const hir::Expression &expression) {
  return ExpressionGenerator{m_ctx}(expression);
}

Handle StatementGenerator::operator()(const hir::LetBinding &let_binding) {
  LetBindingGenerator{m_ctx}(let_binding);

  return {};
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
