//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of top-level item generator.
//*
//*
//****************************************************************************

#include <algorithm>
#include <codegen/basic_block.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/handle.hpp>
#include <codegen/instructions.hpp>
#include <codegen/let_binding_generator.hpp>
#include <codegen/module.hpp>
#include <codegen/parameter.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/top_item_generator.hpp>
#include <codegen/types.hpp>
#include <hir/types.hpp>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <codegen/context.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void TopItemDeclarationCollector::operator()(
    const hir::FunctionDef &function_def) {
  m_ctx.symbols().define_global(function_def.m_signature.m_function_id);
}

void TopItemDeclarationCollector::operator()(
    const hir::ExternFunctionDeclaration &extern_func) {
  m_ctx.symbols().define_global(extern_func.m_signature.m_function_id);
}

void TopItemDeclarationCollector::operator()(const hir::LetBinding &) {}

FunctionDeclaration
TopItemGenerator::generate(const hir::FunctionDeclaration &func_declaration) {
  std::vector<Parameter> parameters;
  std::transform(
      func_declaration.m_parameters.begin(),
      func_declaration.m_parameters.end(), std::back_inserter(parameters),
      [this](const hir::Identifier &parameter) -> Parameter {
        auto handle = m_ctx.symbols().define_parameter(parameter.m_name);
        return Parameter{.m_name = std::move(handle),
                         .m_type = to_llvm_type(
                             m_ctx.type_registry().get(parameter.m_type))};
      });
  return FunctionDeclaration{
      .m_function_id = GlobalHandle{func_declaration.m_function_id},
      .m_return_type = to_llvm_type(m_ctx.type_registry().get(
          std::get<hir::FunctionType>(
              m_ctx.type_registry().get(func_declaration.m_type))
              .m_return_type)),
      .m_parameters = std::move(parameters)};
}

void TopItemGenerator::operator()(const hir::FunctionDef &function_def) {
  ScopeGuard guard(m_ctx.symbols());

  auto &function =
      m_ctx.module().new_function(generate(function_def.m_signature));
  m_ctx.module().set_current_function(function);

  auto return_value = ExpressionGenerator{m_ctx}(function_def.m_body);

  // Wherever we are, we need to add this terminal to the final
  function.current_basic_block().add_terminal(ReturnInstruction{
      .m_value = return_value,
      .m_type = to_llvm_type(m_ctx.type_registry().get(
          std::get<hir::FunctionType>(
              m_ctx.type_registry().get(function_def.m_signature.m_type))
              .m_return_type))});
}

void TopItemGenerator::operator()(
    const hir::ExternFunctionDeclaration &extern_func) {
  ScopeGuard guard(m_ctx.symbols());

  m_ctx.module().add_extern_function_declaration(
      std::make_unique<FunctionDeclaration>(generate(extern_func.m_signature)));
}

void TopItemGenerator::operator()(const hir::LetBinding &let_binding) {
  // TODO: assumes local for the moment
  LetBindingGenerator{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
