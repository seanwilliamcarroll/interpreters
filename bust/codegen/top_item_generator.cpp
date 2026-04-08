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
#include "codegen/instructions.hpp"
#include "codegen/let_binding_generator.hpp"
#include "codegen/types.hpp"
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

  auto &function = m_ctx.m_module.new_function(
      GlobalHandle{function_def.m_function_id},
      to_llvm_type(function_def.m_type->m_return_type));
  m_ctx.m_module.set_current_function(function);

  auto return_value = ExpressionGenerator{m_ctx}(function_def.m_body);

  // Wherever we are, we need to add this terminal to the final
  function.current_basic_block().add_terminal(ReturnInstruction{
      .m_value = return_value,
      .m_type = to_llvm_type(function_def.m_type->m_return_type)});
}

void TopItemGenerator::operator()(const hir::LetBinding &let_binding) {
  LetBindingGenerator{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
