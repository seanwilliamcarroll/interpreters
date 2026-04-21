//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of let-binding generator.
//*
//*
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/context.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function.hpp>
#include <codegen/instructions.hpp>
#include <codegen/let_binding_generator.hpp>
#include <codegen/symbol_table.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void LetBindingGenerator::generate(const zir::LetBinding &let_binding) {
  auto value_handle =
      ExpressionGenerator{m_ctx}.generate(let_binding.m_expression);

  auto binding = m_ctx.arena().get(let_binding.m_identifier);

  auto identifier_handle =
      m_ctx.builder().add_alloca(binding.m_name, m_ctx.to_type(binding.m_type));

  m_ctx.builder().create_store(
      identifier_handle,
      {.m_name = value_handle, .m_type = m_ctx.to_type(binding.m_type)});
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
