//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the top-level item lowerer.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <zir/arena.hpp>
#include <zir/expression_lowerer.hpp>
#include <zir/let_binding_lowerer.hpp>
#include <zir/nodes.hpp>
#include <zir/top_item_lowerer.hpp>
#include <zir/types.hpp>

#include <string>
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
  auto new_type_id = m_ctx.convert(function_def.m_signature.m_type);

  auto binding = Binding{.m_name = function_def.m_signature.m_function_id,
                         .m_type = new_type_id};
  auto binding_id = m_ctx.m_arena.push(std::move(binding));

  auto new_block = ExpressionLowerer{m_ctx}.lower(function_def.m_body);

  std::vector<BindingId> parameters;
  parameters.reserve(function_def.m_signature.m_parameters.size());
  for (const auto &parameter : function_def.m_signature.m_parameters) {
    parameters.emplace_back(ExpressionLowerer{m_ctx}.lower(parameter).m_id);
  }

  return FunctionDef{.m_id = binding_id,
                     .m_parameters = std::move(parameters),
                     .m_body = std::move(new_block)};
}

TopItem TopItemLowerer::TopItemLowerer::operator()(
    const hir::ExternFunctionDeclaration &extern_function_declaration) {
  auto new_type_id =
      m_ctx.convert(extern_function_declaration.m_signature.m_type);

  auto binding =
      Binding{.m_name = extern_function_declaration.m_signature.m_function_id,
              .m_type = new_type_id};
  auto binding_id = m_ctx.m_arena.push(std::move(binding));

  // All we need is the binding id, since we don't even need the parameter
  // names, just their types for a call site later on
  return ExternFunctionDeclaration{.m_id = binding_id};
}

TopItem TopItemLowerer::operator()(const hir::LetBinding &let_binding) {
  return LetBindingLowerer{m_ctx}.lower(let_binding);
}

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
