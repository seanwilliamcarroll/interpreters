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
#include "codegen/function.hpp"
#include "codegen/handle.hpp"
#include "codegen/instructions.hpp"
#include "codegen/let_binding_generator.hpp"
#include "codegen/symbol_table.hpp"
#include "codegen/types.hpp"
#include "hir/types.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void TopItemDeclarationCollector::operator()(
    const hir::FunctionDef &function_def) {
  m_ctx.m_symbol_table.define_global(function_def.m_function_id);
}

void TopItemDeclarationCollector::operator()(const hir::LetBinding &) {}

void TopItemGenerator::operator()(const hir::FunctionDef &function_def) {
  ScopeGuard guard(m_ctx.m_symbol_table);
  auto &function = m_ctx.m_module.new_function(FunctionDeclaration{
      .m_function_id = GlobalHandle{function_def.m_function_id},
      .m_return_type = to_llvm_type(function_def.m_type->m_return_type)});
  m_ctx.m_module.set_current_function(function);

  for (const auto &parameter : function_def.m_parameters) {
    auto handle = m_ctx.m_symbol_table.define_parameter(parameter.m_name);

    function.signature().add_parameter(ParameterHandle{parameter.m_name},
                                       to_llvm_type(parameter.m_type));
  }

  auto return_value = ExpressionGenerator{m_ctx}(function_def.m_body);

  // Wherever we are, we need to add this terminal to the final
  function.current_basic_block().add_terminal(ReturnInstruction{
      .m_value = return_value,
      .m_type = to_llvm_type(function_def.m_type->m_return_type)});
}

void TopItemGenerator::operator()(const hir::LetBinding &let_binding) {
  // TODO: assumes local for the moment
  LetBindingGenerator{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
