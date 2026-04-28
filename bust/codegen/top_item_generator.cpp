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
#include <codegen/instructions.hpp>
#include <codegen/let_binding_generator.hpp>
#include <codegen/module.hpp>
#include <codegen/naming_conventions.hpp>
#include <codegen/parameter.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/top_item_generator.hpp>
#include <codegen/value.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
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
  auto callee = Value{
      .m_handle =
          GlobalHandle{
              .m_handle = binding.m_name,
          },
      .m_type_id = m_ctx.m_ptr,
  };

  auto function_type = m_ctx.arena().as_function(binding.m_type);

  auto return_type_id = m_ctx.to_type(function_type.m_return_type);

  std::vector<TypeId> parameter_types;
  parameter_types.reserve(function_type.m_parameters.size());
  for (const auto &parameter_type : function_type.m_parameters) {
    parameter_types.emplace_back(m_ctx.to_type(parameter_type));
  }

  m_ctx.define_function(binding.m_name, std::move(callee), return_type_id,
                        std::move(parameter_types));
}

void TopItemDeclarationCollector::operator()(
    const zir::ExternFunctionDeclaration &extern_func) {
  const auto &binding = m_ctx.arena().get(extern_func.m_id);
  auto callee = Value{
      .m_handle =
          GlobalHandle{
              .m_handle = binding.m_name,
          },
      .m_type_id = m_ctx.m_ptr,
  };

  auto function_type = m_ctx.arena().as_function(binding.m_type);

  auto return_type_id = m_ctx.to_type(function_type.m_return_type);

  std::vector<TypeId> parameter_types;
  parameter_types.reserve(function_type.m_parameters.size());
  for (const auto &parameter_type : function_type.m_parameters) {
    parameter_types.emplace_back(m_ctx.to_type(parameter_type));
  }

  m_ctx.define_function(binding.m_name, std::move(callee), return_type_id,
                        std::move(parameter_types));
}

void TopItemDeclarationCollector::operator()(
    const zir::LetBinding & /*unused*/) {}

void TopItemGenerator::generate(const zir::TopItem &top_item) {
  std::visit(*this, top_item);
}

FunctionDeclaration
TopItemGenerator::generate_signature(const zir::FunctionDef &function_def) {
  const auto &zir_binding = m_ctx.arena().get(function_def.m_id);
  auto binding = m_ctx.symbols().lookup(zir_binding.m_name);
  const auto &function_binding = std::get<FunctionBinding>(binding);

  std::vector<Parameter> parameters;
  std::ranges::transform(
      function_def.m_parameters, std::back_inserter(parameters),
      [&](const auto &id) -> Parameter {
        const auto &parameter_binding = m_ctx.arena().get(id);
        return Parameter{.m_name = parameter_binding.m_name,
                         .m_type = m_ctx.to_type(parameter_binding.m_type)};
      });
  return {.m_function_id = function_binding.m_callee,
          .m_return_type = function_binding.m_return_type,
          .m_parameters = std::move(parameters)};
}

FunctionDeclaration TopItemGenerator::generate_signature(
    const zir::ExternFunctionDeclaration &extern_function_declaration) const {
  const auto &zir_binding = m_ctx.arena().get(extern_function_declaration.m_id);
  auto binding = m_ctx.symbols().lookup(zir_binding.m_name);
  const auto &function_binding = std::get<FunctionBinding>(binding);

  std::vector<Parameter> parameters;
  for (auto [index, type_id] : std::views::zip(
           std::views::iota(0ULL), function_binding.m_parameter_types)) {
    // Don't actually need names on externs, but doesn't hurt
    parameters.emplace_back(Parameter{
        .m_name = conventions::make_param_name(index), .m_type = type_id});
  }
  return {.m_function_id = function_binding.m_callee,
          .m_return_type = function_binding.m_return_type,
          .m_parameters = std::move(parameters)};
}

void TopItemGenerator::operator()(const zir::FunctionDef &function_def) {
  ScopeGuard guard(m_ctx.symbols());

  const auto &binding = m_ctx.arena().get(function_def.m_id);

  auto signature = generate_signature(function_def);
  auto function = m_ctx.builder().make_function(signature);
  m_ctx.builder().enter_function(function);

  m_ctx.emit_parameter_prologue(signature.m_parameters);

  auto return_value = ExpressionGenerator{m_ctx}.generate(function_def.m_body);

  const auto &return_type_id =
      m_ctx.arena().as_function(binding.m_type).m_return_type;

  if (return_type_id == m_ctx.arena().m_unit) {
    m_ctx.builder().emit_return_void();
  } else {
    // Wherever we are, we need to add this terminal to the final
    m_ctx.builder().emit_return(return_value);
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
