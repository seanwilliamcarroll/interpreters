//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of top-level item generator.
//*
//*
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/context.hpp>
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
#include <zir/arena.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************
void TopItemDeclarationCollector::collect(const zir::TopItem &top_item) {
  std::visit(*this, top_item);
}

void TopItemDeclarationCollector::operator()(
    const zir::FunctionDef &function_def) {
  const auto &binding = m_ctx.arena().get(function_def.m_id);
  m_ctx.symbols().define_global(binding.m_name);
}

void TopItemDeclarationCollector::operator()(
    const zir::ExternFunctionDeclaration &extern_func) {
  const auto &binding = m_ctx.arena().get(extern_func.m_id);
  m_ctx.symbols().define_global(binding.m_name);
}

void TopItemDeclarationCollector::operator()(
    const zir::LetBinding & /*unused*/) {}

void TopItemGenerator::generate(const zir::TopItem &top_item) {
  std::visit(*this, top_item);
}

FunctionDeclaration
TopItemGenerator::generate_signature(const zir::FunctionDef &function_def) {
  const auto &binding = m_ctx.arena().get(function_def.m_id);
  const auto &type = m_ctx.arena().as_function(binding.m_type);

  std::vector<Parameter> parameters;
  std::ranges::transform(
      function_def.m_parameters, std::back_inserter(parameters),
      [&](const auto &id) -> Parameter {
        const auto &parameter_binding = m_ctx.arena().get(id);
        auto handle =
            m_ctx.symbols().define_parameter(parameter_binding.m_name);
        return Parameter{.m_name = std::move(handle),
                         .m_type = m_ctx.to_type(parameter_binding.m_type)};
      });
  return {.m_function_id = GlobalHandle{binding.m_name},
          .m_return_type = m_ctx.to_type(type.m_return_type),
          .m_parameters = std::move(parameters)};
}

FunctionDeclaration TopItemGenerator::generate_signature(
    const zir::ExternFunctionDeclaration &extern_function_declaration) {
  const auto &binding = m_ctx.arena().get(extern_function_declaration.m_id);
  const auto &type = m_ctx.arena().as_function(binding.m_type);

  std::vector<Parameter> parameters;
  for (auto [index, type_id] :
       std::views::zip(std::views::iota(0ULL), type.m_parameters)) {
    // Don't actually need names on externs, but doesn't hurt
    auto handle =
        m_ctx.symbols().define_parameter("param_" + std::to_string(index));
    parameters.emplace_back(
        Parameter{.m_name = handle, .m_type = m_ctx.to_type(type_id)});
  }
  return {.m_function_id = GlobalHandle{binding.m_name},
          .m_return_type = m_ctx.to_type(type.m_return_type),
          .m_parameters = std::move(parameters)};
}

void TopItemGenerator::operator()(const zir::FunctionDef &function_def) {
  ScopeGuard guard(m_ctx.symbols());

  auto &function =
      m_ctx.module().new_function(generate_signature(function_def));
  m_ctx.module().set_current_function(function);

  auto return_value = ExpressionGenerator{m_ctx}.generate(function_def.m_body);

  const auto &binding = m_ctx.arena().get(function_def.m_id);

  const auto &return_type_id =
      m_ctx.arena().as_function(binding.m_type).m_return_type;

  if (return_type_id == m_ctx.arena().type().m_unit) {
    function.current_basic_block().add_terminal(ReturnVoidInstruction{});
  } else {
    // Wherever we are, we need to add this terminal to the final
    function.current_basic_block().add_terminal(ReturnInstruction{
        .m_value = return_value, .m_type = m_ctx.to_type(return_type_id)});
  }
}

void TopItemGenerator::operator()(
    const zir::ExternFunctionDeclaration &extern_func) {
  ScopeGuard guard(m_ctx.symbols());

  m_ctx.module().add_extern_function_declaration(
      std::make_unique<FunctionDeclaration>(generate_signature(extern_func)));
}

void TopItemGenerator::operator()(const zir::LetBinding &let_binding) {
  // TODO: assumes local for the moment
  LetBindingGenerator{m_ctx}.generate(let_binding);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
