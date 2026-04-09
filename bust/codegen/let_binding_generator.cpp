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
#include "codegen/types.hpp"
#include "hir/types.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void LetBindingGenerator::operator()(const hir::LetBinding &let_binding) {

  auto value_handle = ExpressionGenerator{m_ctx}(let_binding.m_expression);

  // TODO: assumes local for the moment
  auto identifier_handle =
      m_ctx.m_symbol_table.define_local(let_binding.m_variable.m_name);

  m_ctx.current_function().add_alloca_instruction(AllocaInstruction{
      .m_handle = identifier_handle,
      .m_type = to_llvm_type(let_binding.m_expression.m_type)});

  m_ctx.current_basic_block().add_instruction(StoreInstruction{
      .m_destination = identifier_handle,
      .m_source = value_handle,
      .m_type = to_llvm_type(let_binding.m_expression.m_type),
  });
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
