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
    const ast::FunctionDef &function_def) {
  if (auto other_id = m_ctx.m_env.lookup(function_def.m_id.m_name)) {
    throw core::CompilerException(
        "TypeChecker",
        "Cannot redefine identifier!\nAlready defined " +
            function_def.m_id.m_name +
            " with type: " + other_id.value().m_type + " at " +
            type_location(other_id.value().m_type),
        function_def.m_id.m_location);
  }

  auto return_type = TypeConverter{m_ctx}.get_type(function_def.m_return_type);
  if (std::holds_alternative<TypeVariable>(return_type)) {
    // Currently illegal
    throw core::CompilerException(
        "TypeChecker", std::string("UNIMPLEMENTED") + " " + __PRETTY_FUNCTION__,
        type_location(return_type));
  }

  auto [_, parameter_types] =
      TypeConverter{m_ctx}.convert_parameters(function_def.m_parameters);

  auto function_type = std::make_shared<FunctionTypePtr::element_type>(
      FunctionType{{function_def.m_location},
                   std::move(parameter_types),
                   std::move(return_type)});

  Type function_type_as_type = std::move(function_type);
  m_ctx.m_env.define(function_def.m_id.m_name, function_type_as_type);
}

TopItem TopItemChecker::operator()(const ast::FunctionDef &function_def) {
  // Should throw, this would be an error of the type checker itself
  auto maybe_function_type = m_ctx.m_env.lookup(function_def.m_id.m_name);
  if (!maybe_function_type.has_value()) {
    throw core::CompilerException("TypeChecker",
                                  "Compiler error: should have defined " +
                                      function_def.m_id.m_name +
                                      " in first pass!",
                                  function_def.m_location);
  }
  auto function_type = maybe_function_type.value().m_type;

  auto [parameters, _] =
      TypeConverter{m_ctx}.convert_parameters(function_def.m_parameters);

  // Define the function in the environment before checking its body
  // (allows recursion)
  const auto &expected_return_type =
      std::get<FunctionTypePtr>(function_type)->m_return_type;

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
                     function_def.m_id.m_name,
                     std::move(std::get<FunctionTypePtr>(function_type)),
                     std::move(parameters),
                     std::move(body)};
}

TopItem TopItemChecker::operator()(const ast::LetBinding &let_binding) {
  return LetBindingChecker{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
