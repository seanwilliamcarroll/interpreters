//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement and block checker implementation.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <hir/context.hpp>
#include <hir/environment.hpp>
#include <hir/expression_checker.hpp>
#include <hir/free_type_variable_collector.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/nodes.hpp>
#include <hir/statement_checker.hpp>
#include <hir/type_converter.hpp>
#include <hir/type_unifier.hpp>
#include <hir/type_variable_collapser.hpp>
#include <source_location.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

Statement StatementChecker::operator()(const ast::LetBinding &let_binding) {
  // Evaluate the expression's type in the current scope
  // Compare with a type annotation if one is provided
  // Create new LetBinding

  auto body =
      ExpressionChecker{m_ctx}.check_expression(let_binding.m_expression);

  auto annotated_type = TypeConverter{m_ctx}.get_type(let_binding.m_variable);

  // If annotated type is Unknown, go with type of expression
  // Else, unify the two (throws on mismatch)
  try {
    m_ctx.type_unifier().unify(annotated_type, body.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        let_binding.m_location);
  }

  if (m_ctx.is_type_variable(annotated_type) &&
      body.m_type == m_ctx.type_arena().m_never) {
    // We've got an unannotated lambda whose body is a never type
    // Explicitly set the type to the unit type
    annotated_type = m_ctx.type_arena().m_unit;
  }

  auto unified_type = m_ctx.type_unifier().find(annotated_type);

  auto collapsed_type =
      collapse_types(m_ctx.type_arena(), m_ctx.type_unifier(), unified_type);

  auto binding_id = m_ctx.next_let_binding_id();

  auto new_identifier = Identifier{
      .m_location = let_binding.m_variable.m_location,
      .m_name = let_binding.m_variable.m_name,
      .m_id = binding_id,
      .m_type = collapsed_type,
  };

  auto free_variables = collect_free_variables(m_ctx, new_identifier.m_type);

  // Store the new let binding
  m_ctx.env().define(new_identifier.m_name, binding_id,
                     TypeScheme{
                         .m_type = new_identifier.m_type,
                         .m_free_type_variables = free_variables,
                     },
                     let_binding.m_is_mutable);

  return LetBinding{
      .m_location = let_binding.m_location,
      .m_variable = std::move(new_identifier),
      .m_expression = std::move(body),
      .m_is_mutable = let_binding.m_is_mutable,
  };
}

Statement StatementChecker::operator()(const ast::Expression &expression) {
  return ExpressionChecker{m_ctx}.check_expression(expression);
}

std::optional<Place> try_lower_place(const Expression &lhs) {
  return std::visit(
      [&](auto &&l) -> std::optional<Place> {
        using T = std::decay_t<decltype(l)>;
        if constexpr (std::is_same_v<T, Identifier>) {
          return {l};
        } else {
          return {};
        }
      },
      lhs.m_expression);
}

bool StatementChecker::is_place_mutable(const Place &place) {
  return std::visit(
      [&](auto &&p) -> bool {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, Identifier>) {
          auto maybe_binding = m_ctx.env().lookup(p.m_name);
          if (!maybe_binding.has_value()) {
            throw core::CompilerException(
                "TypeChecker", "Unknown identifier: " + p.m_name, p.m_location);
          }
          const auto &binding = maybe_binding.value();
          return binding.m_is_mutable;
        }
      },
      place);
}

Statement StatementChecker::operator()(const ast::Assignment &assignment) {
  // Need to check if we can lower the lhs Expression to a Place
  auto lhs = ExpressionChecker{m_ctx}.check_expression(assignment.m_lhs);
  auto maybe_place = try_lower_place(lhs);
  if (!maybe_place.has_value()) {
    throw core::CompilerException("TypeChecker",
                                  "Unable to lower lhs to a hir::Place!",
                                  assignment.m_location);
  }
  auto place = std::move(maybe_place.value());

  if (!is_place_mutable(place)) {
    throw core::CompilerException("TypeChecker",
                                  "Cannot assign to immutable lhs!",
                                  assignment.m_location);
  }

  auto rhs = ExpressionChecker{m_ctx}.check_expression(assignment.m_rhs);

  // Need to unify these two
  try {
    m_ctx.type_unifier().unify(lhs.m_type, rhs.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        assignment.m_location);
  }

  return Assignment{
      .m_location = assignment.m_location,
      .m_place = std::move(place),
      .m_expression = std::move(rhs),
  };
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
