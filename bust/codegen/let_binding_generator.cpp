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
  auto value = ExpressionGenerator{m_ctx}.generate(let_binding.m_expression);

  auto zir_binding = m_ctx.arena().get(let_binding.m_identifier);

  m_ctx.define_local(zir_binding.m_name, m_ctx.to_type(zir_binding.m_type),
                     value);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
