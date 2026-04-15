//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item zonker implementation.
//*
//*
//****************************************************************************

#include "exceptions.hpp"
#include "hir/nodes.hpp"
#include "zonk/expression_zonker.hpp"
#include "zonk/let_binding_zonker.hpp"
#include <variant>
#include <zonk/top_item_zonker.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

hir::TopItem TopItemZonker::zonk(hir::TopItem top_item) {
  return std::visit(*this, std::move(top_item));
}

hir::FunctionDeclaration
TopItemZonker::zonk(hir::FunctionDeclaration function_declaration) {
  auto zonker = ExpressionZonker{m_ctx};

  // For now, we expect top level functions to have annotated parameters and
  // return types
  std::vector<hir::Identifier> zonked_parameters;
  zonked_parameters.reserve(function_declaration.m_parameters.size());
  std::vector<hir::TypeId> zonked_parameter_ids;
  zonked_parameter_ids.reserve(function_declaration.m_parameters.size());
  for (auto &parameter : function_declaration.m_parameters) {
    auto zonked_expression = zonker(std::move(parameter));
    if (!std::holds_alternative<hir::Identifier>(zonked_expression)) {
      throw core::InternalCompilerError("Bad expression visitor");
    }
    auto &zonked_parameter = std::get<hir::Identifier>(zonked_expression);
    zonked_parameter_ids.push_back(zonked_parameter.m_type);
    zonked_parameters.push_back(std::move(zonked_parameter));
  }

  // Insert the return value into the new type registry
  auto zonked_type_id = m_ctx.find_and_register(function_declaration.m_type);

  const auto &zonked_return_type_id =
      std::get<hir::FunctionType>(m_ctx.m_new_type_registry.get(zonked_type_id))
          .m_return_type;

  // Reconstruct the zonked function type
  auto zonked_function_type =
      hir::FunctionType{.m_parameters = std::move(zonked_parameter_ids),
                        .m_return_type = zonked_return_type_id};
  auto zonked_function_type_id =
      m_ctx.m_new_type_registry.intern(zonked_function_type);

  return {std::move(function_declaration.m_function_id),
          function_declaration.m_id, zonked_function_type_id,
          std::move(zonked_parameters)};
}

hir::TopItem TopItemZonker::operator()(hir::FunctionDef function_def) {
  auto zonker = ExpressionZonker{m_ctx};

  auto zonked_signature = zonk(std::move(function_def.m_signature));

  // Zonk the body
  auto zonked_body = zonker.zonk_block(std::move(function_def.m_body));

  return hir::FunctionDef{{function_def.m_location},
                          std::move(zonked_signature),
                          std::move(zonked_body)};
}

hir::TopItem
TopItemZonker::operator()(hir::ExternFunctionDeclaration extern_func) {
  auto zonked_signature = zonk(std::move(extern_func.m_signature));

  return hir::ExternFunctionDeclaration{{extern_func.m_location},
                                        std::move(zonked_signature)};
}

hir::TopItem TopItemZonker::operator()(hir::LetBinding let_binding) {
  return LetBindingZonker{m_ctx}.zonk(std::move(let_binding));
}

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
