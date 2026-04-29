//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the top-level item lowerer.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <zir/context.hpp>
#include <zir/environment.hpp>
#include <zir/expression_lowerer.hpp>
#include <zir/nodes.hpp>
#include <zir/top_item_lowerer.hpp>

#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

TopItem TopItemLowerer::lower(const hir::TopItem &top_item) {
  return std::visit(*this, top_item);
}

TopItem TopItemLowerer::TopItemLowerer::operator()(
    const hir::FunctionDef &function_def) {
  auto binding_id =
      m_ctx.get_global_binding(function_def.m_signature.m_function_id);

  // Push scope before we define the parameters
  ScopeGuard guard{m_ctx.env()};

  std::vector<BindingId> parameters;
  parameters.reserve(function_def.m_signature.m_parameters.size());
  for (const auto &parameter : function_def.m_signature.m_parameters) {
    auto new_identifier = ExpressionLowerer{m_ctx}.lower_definition(parameter);
    m_ctx.env().define(parameter.m_name, new_identifier.m_id);
    parameters.emplace_back(new_identifier.m_id);
  }

  auto new_block = ExpressionLowerer{m_ctx}.lower(function_def.m_body);

  return FunctionDef{.m_id = binding_id,
                     .m_parameters = std::move(parameters),
                     .m_body = std::move(new_block)};
}

TopItem TopItemLowerer::TopItemLowerer::operator()(
    const hir::ExternFunctionDeclaration &extern_function_declaration) const {
  auto binding_id = m_ctx.get_global_binding(
      extern_function_declaration.m_signature.m_function_id);

  // All we need is the binding id, since we don't even need the parameter
  // names, just their types for a call site later on
  return ExternFunctionDeclaration{.m_id = binding_id};
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
