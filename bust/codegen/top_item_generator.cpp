//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of top-level item generator.
//*
//*
//****************************************************************************

#include "codegen/top_item_generator.hpp"
#include "codegen/expression_generator.hpp"
#include "codegen/let_binding_generator.hpp"
#include "exceptions.hpp"
#include "hir/types.hpp"
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void TopItemGenerator::operator()(const hir::FunctionDef &function_def) {
  if (!function_def.m_parameters.empty()) {
    throw core::CompilerException("Codegen", "UNIMPLEMENTED",
                                  function_def.m_location);
  }

  auto &function = m_ctx.new_function();

  function.m_function_id = function_def.m_function_id;
  function.m_return_type =
      hir::type_to_string(function_def.m_type->m_return_type);

  auto &final_block = function.new_basic_block();

  auto return_value = ExpressionGenerator{m_ctx}(function_def.m_body);

  final_block.m_terminal_instruction = ReturnInstruction{
      .m_type = hir::type_to_string(function_def.m_type->m_return_type),
      .m_value = return_value};
}

void TopItemGenerator::operator()(const hir::LetBinding &let_binding) {
  LetBindingGenerator{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
