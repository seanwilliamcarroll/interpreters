//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of let-binding generator.
//*
//*
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function.hpp>
#include <codegen/instructions.hpp>
#include <codegen/let_binding_generator.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>

#include <codegen/context.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void LetBindingGenerator::operator()(const hir::LetBinding &let_binding) {
  auto value_handle = ExpressionGenerator{m_ctx}(let_binding.m_expression);

  auto identifier_handle =
      m_ctx.symbols().define_local(let_binding.m_variable.m_name);

  m_ctx.function().add_alloca_instruction(
      AllocaInstruction{.m_handle = identifier_handle,
                        .m_type = to_llvm_type(m_ctx.type_arena().get(
                            let_binding.m_expression.m_type))});

  m_ctx.block().add_instruction(StoreInstruction{
      .m_destination = identifier_handle,
      .m_source = value_handle,
      .m_type =
          to_llvm_type(m_ctx.type_arena().get(let_binding.m_expression.m_type)),
  });
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
