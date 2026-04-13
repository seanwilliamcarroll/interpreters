//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item checker implementation.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <hir/block_checker.hpp>
#include <hir/context.hpp>
#include <hir/environment.hpp>
#include <hir/let_binding_checker.hpp>
#include <hir/nodes.hpp>
#include <hir/top_item_checker.hpp>
#include <hir/type_converter.hpp>
#include <hir/type_unifier.hpp>
#include <hir/types.hpp>
#include <memory>
#include <optional>
#include <source_location.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <ast/nodes.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

void TopItemChecker::collect_function_signature(
    const ast::FunctionDeclaration &declaration) {
  if (auto other_id = m_ctx.m_env.lookup(declaration.m_id.m_name)) {
    throw core::CompilerException(
        "TypeChecker",
        "Cannot redefine identifier!\nAlready defined " +
            declaration.m_id.m_name + " with type: " +
            m_ctx.m_type_registry.to_string(other_id.value().m_type),
        declaration.m_id.m_location);
  }

  auto return_type_id =
      TypeConverter{m_ctx}.get_type(declaration.m_return_type);
  const auto &return_type = m_ctx.m_type_registry.get(return_type_id);
  if (std::holds_alternative<TypeVariable>(return_type)) {
    throw core::InternalCompilerError(
        "return type inference for top-level functions not yet implemented");
  }

  auto [_, parameter_types] =
      TypeConverter{m_ctx}.convert_parameters(declaration.m_parameters);

  auto function_type_id = m_ctx.m_type_registry.intern(
      FunctionType{std::move(parameter_types), return_type_id});

  m_ctx.m_env.define(declaration.m_id.m_name, function_type_id);
}

TopItem TopItemChecker::operator()(const ast::FunctionDef &function_def) {
  // Should throw, this would be an error of the type checker itself
  auto maybe_function_type =
      m_ctx.m_env.lookup(function_def.m_signature.m_id.m_name);
  if (!maybe_function_type.has_value()) {
    throw core::InternalCompilerError(
        "function '" + function_def.m_signature.m_id.m_name +
        "' not found after first pass signature collection");
  }
  auto function_type_id = maybe_function_type.value().m_type;

  auto [parameters, _] = TypeConverter{m_ctx}.convert_parameters(
      function_def.m_signature.m_parameters);

  // Define the function in the environment before checking its body
  // (allows recursion)
  auto expected_return_type =
      std::get<FunctionType>(m_ctx.m_type_registry.get(function_type_id))
          .m_return_type;

  auto body = BlockChecker{m_ctx}.check_callable_body(
      parameters, expected_return_type, function_def.m_body);

  try {
    m_ctx.m_type_unifier.unify(body.m_type, expected_return_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error: ") + error.what(),
        function_def.m_location);
  }

  return FunctionDef{{function_def.m_location},
                     function_def.m_signature.m_id.m_name,
                     function_type_id,
                     std::move(parameters),
                     std::move(body)};
}

TopItem
TopItemChecker::operator()(const ast::ExternFunctionDeclaration & // extern_func
) {
  return {};
}

TopItem TopItemChecker::operator()(const ast::LetBinding &let_binding) {
  return LetBindingChecker{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
