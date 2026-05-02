//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of statement generator.
//*
//*
//****************************************************************************

#include <codegen/context.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/statement_generator.hpp>
#include <codegen/value.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>

#include <variant>
#include <vector>

#include "codegen/symbol_table.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Value StatementGenerator::generate(const zir::Statement &statement) {
  return std::visit(*this, statement);
}

Value StatementGenerator::operator()(
    const zir::ExpressionStatement &expression) {
  return ExpressionGenerator{m_ctx}.generate(expression.m_expression);
}

Value StatementGenerator::operator()(const zir::LetBinding &let_binding) {
  auto value = ExpressionGenerator{m_ctx}.generate(let_binding.m_expression);

  auto zir_binding = m_ctx.arena().get(let_binding.m_identifier);

  auto maybe_lambda = m_ctx.arena().get(let_binding.m_expression).m_expr_kind;

  // Note: this is a bandaid, won't work when there is any sort of indirection
  // around the closure
  bool is_closure_rhs =
      std::holds_alternative<zir::LambdaExpr>(maybe_lambda) &&
      !std::get<zir::LambdaExpr>(maybe_lambda).m_captures.empty();

  if (is_closure_rhs) {
    m_ctx.define_local_closure(zir_binding.m_name, value);
  } else {
    m_ctx.define_local(zir_binding.m_name, m_ctx.to_type(zir_binding.m_type),
                       value);
  }
  return {};
}

AllocaBinding StatementGenerator::generate(const zir::Place &place) {
  return std::visit(
      [&](const auto &p) -> AllocaBinding {
        using T = std::decay_t<decltype(p)>;
        if constexpr (std::is_same_v<T, zir::IdentifierExpr>) {
          auto binding = m_ctx.arena().get(p.m_id);
          auto probably_alloca = m_ctx.symbols().lookup(binding.m_name);
          return std::get<AllocaBinding>(probably_alloca);
        }
      },
      place);
}

Value StatementGenerator::operator()(const zir::Assignment &assignment) {
  auto value = ExpressionGenerator{m_ctx}.generate(assignment.m_expression);

  auto alloca = generate(assignment.m_place);

  m_ctx.builder().emit_store(alloca.m_ptr, value);

  // Just like a let binding, we emit a unit value here
  return {};
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
