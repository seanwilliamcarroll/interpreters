//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item checker implementation.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <hir/block_checker.hpp>
#include <hir/context.hpp>
#include <hir/environment.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/nodes.hpp>
#include <hir/top_item_checker.hpp>
#include <hir/type_arena.hpp>
#include <hir/type_converter.hpp>
#include <hir/type_unifier.hpp>
#include <hir/types.hpp>
#include <source_location.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

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
            m_ctx.to_string(other_id.value().m_type_scheme.m_type),
        declaration.m_id.m_location);
  }

  auto return_type_id =
      TypeConverter{m_ctx}.get_type(declaration.m_return_type);
  const auto &return_type = m_ctx.m_type_arena.get(return_type_id);
  if (std::holds_alternative<TypeVariable>(return_type)) {
    throw core::InternalCompilerError(
        "return type inference for top-level functions not yet implemented");
  }

  auto [_, parameter_types] =
      TypeConverter{m_ctx}.convert_parameters(declaration.m_parameters);

  auto function_type_id = m_ctx.m_type_arena.intern(
      FunctionType{.m_parameters = std::move(parameter_types),
                   .m_return_type = return_type_id});

  m_ctx.m_env.define(declaration.m_id.m_name, m_ctx.next_let_binding_id(),
                     function_type_id);
}

hir::FunctionDeclaration TopItemChecker::check_declaration(
    const ast::FunctionDeclaration &function_declaration) {
  // Should throw, this would be an error of the type checker itself
  auto maybe_function_type =
      m_ctx.m_env.lookup(function_declaration.m_id.m_name);
  if (!maybe_function_type.has_value()) {
    throw core::InternalCompilerError(
        "function '" + function_declaration.m_id.m_name +
        "' not found after first pass signature collection");
  }
  const auto &[function_id, function_type_scheme] = maybe_function_type.value();
  auto function_type_id = function_type_scheme.m_type;

  auto [parameters, _] = TypeConverter{m_ctx}.convert_parameters(
      function_declaration.m_parameters);

  return FunctionDeclaration{.m_function_id = function_declaration.m_id.m_name,
                             .m_id = function_id,
                             .m_type = function_type_id,
                             .m_parameters = std::move(parameters)};
}

TopItem TopItemChecker::operator()(const ast::FunctionDef &function_def) {
  auto signature = check_declaration(function_def.m_signature);

  // Define the function in the environment before checking its body
  // (allows recursion)
  auto expected_return_type = m_ctx.as_function(signature.m_type).m_return_type;

  auto body = BlockChecker{m_ctx}.check_callable_body(
      signature.m_parameters, expected_return_type, function_def.m_body);

  try {
    m_ctx.m_type_unifier.unify(body.m_type, expected_return_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error: ") + error.what(),
        function_def.m_location);
  }

  return FunctionDef{
      {function_def.m_location}, std::move(signature), std::move(body)};
}

TopItem
TopItemChecker::operator()(const ast::ExternFunctionDeclaration &extern_func) {
  auto signature = check_declaration(extern_func.m_signature);

  return ExternFunctionDeclaration{{extern_func.m_location},
                                   std::move(signature)};
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
