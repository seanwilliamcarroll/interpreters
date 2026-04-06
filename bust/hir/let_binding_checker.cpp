//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Let binding checker implementation.
//*
//*
//****************************************************************************

#include "hir/let_binding_checker.hpp"
#include "ast/nodes.hpp"
#include "exceptions.hpp"
#include "hir/expression_checker.hpp"
#include "hir/nodes.hpp"
#include "hir/type_converter.hpp"

//****************************************************************************
namespace bust::hir {
//****************************************************************************

LetBinding LetBindingChecker::operator()(const ast::LetBinding &let_binding) {
  // Evaluate the expression's type in the current scope
  // Compare with a type annotation if one is provided
  // Create new LetBinding

  auto body = std::visit(ExpressionChecker{m_ctx}, let_binding.m_expression);

  auto annotated_type = TypeConverter{m_ctx}.get_type(let_binding.m_variable);

  // If annotated type is Unknown, go with type of expression
  // Else, unify the two (throws on mismatch)
  try {
    m_ctx.m_type_unifier.unify(annotated_type, body.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        let_binding.m_location);
  }

  auto unified_type = m_ctx.m_type_unifier.find(annotated_type);

  auto new_identifier = Identifier{{let_binding.m_variable.m_location},
                                   let_binding.m_variable.m_name,
                                   std::move(unified_type)};

  FreeTypeVariableCollector collector{};
  std::visit(collector, new_identifier.m_type);

  // Store the new let binding
  m_ctx.m_env.define(new_identifier.m_name,
                     TypeScheme{clone_type(new_identifier.m_type),
                                std::move(collector.m_free_type_variables)});

  return {{let_binding.m_location}, std::move(new_identifier), std::move(body)};
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
