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
#include <codegen/value.hpp>
#include <zir/nodes.hpp>

#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Value StatementGenerator::generate(const zir::Statement &statement) {
  return std::visit(*this, statement);
}

Value StatementGenerator::operator()(
    const zir::ExpressionStatement &expression) {
  return ExpressionGenerator{m_ctx}.generate(expression.m_expression);
}

Value StatementGenerator::operator()(const zir::LetBinding &let_binding) {
  LetBindingGenerator{m_ctx}.generate(let_binding);
  return {};
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
