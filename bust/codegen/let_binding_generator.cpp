//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of let-binding generator.
//*
//*
//****************************************************************************

#include "codegen/let_binding_generator.hpp"
#include "codegen/expression_generator.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void LetBindingGenerator::operator()(const hir::LetBinding &let_binding) {

  auto identifier_handle =
      m_ctx.m_symbol_table.define(let_binding.m_variable.m_name);

  m_ctx.m_output += "  " + identifier_handle + " = alloca " +
                    let_binding.m_expression.m_type + "\n";

  auto value_handle = ExpressionGenerator{m_ctx}(let_binding.m_expression);

  m_ctx.m_output += "  store " + let_binding.m_expression.m_type + " " +
                    value_handle + ", ptr " + identifier_handle + "\n";
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
