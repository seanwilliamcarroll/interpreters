//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Tree-walking evaluator implementation.
//*
//*
//****************************************************************************

#include <evaluator.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <eval/context.hpp>
#include <eval/environment.hpp>
#include <eval/expression_evaluator.hpp>
#include <eval/top_item_evaluator.hpp>
#include <eval/values.hpp>
#include <exceptions.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

int64_t Evaluator::operator()(const hir::Program &program) {
  auto context = eval::Context{{}, program.m_type_registry};

  // Essentially go through program and load functions into env
  for (const auto &top_item : program.m_top_items) {
    // Throw away values for now
    std::visit(eval::TopItemEvaluator{context}, top_item);
  }

  // Then "call" main() from the environment?
  auto main_expr = context.m_env.lookup("main");

  if (!main_expr.has_value() ||
      !std::holds_alternative<eval::Closure>(main_expr.value())) {
    throw core::InternalCompilerError(
        "main not found in environment after type checking");
  }

  auto main_closure = std::get<eval::Closure>(main_expr.value());

  auto final_value = eval::ExpressionEvaluator{context}.evaluate_function_body(
      *main_closure.m_expression);

  if (!std::holds_alternative<eval::I64>(final_value)) {
    throw core::InternalCompilerError(
        "main does not return i64 after type checking");
  }

  auto i64_value = std::get<eval::I64>(final_value);

  return i64_value.m_value;
}

//****************************************************************************
} // namespace bust
//****************************************************************************
