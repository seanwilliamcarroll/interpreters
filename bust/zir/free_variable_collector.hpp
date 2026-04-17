//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Free variable collector for bust ZIR expressions.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <scope_guard.hpp>
#include <zir/context.hpp>
#include <zir/nodes.hpp>

#include <cassert>
#include <iterator>
#include <memory>
#include <unordered_set>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct BoundVariableEnvironment {
  struct Scope {
    explicit Scope(const std::shared_ptr<Scope> &parent) : m_parent(parent) {}

    bool is_present(BindingId id) {
      if (m_bound_identifiers.contains(id)) {
        return true;
      }
      if (m_parent == nullptr) {
        return false;
      }
      return m_parent->is_present(id);
    }

    void define(BindingId id) { m_bound_identifiers.insert(id); }

  private:
    std::unordered_set<BindingId> m_bound_identifiers;
    std::shared_ptr<Scope> m_parent;
  };

  BoundVariableEnvironment() {
    m_scopes.emplace_back(std::make_shared<Scope>(nullptr));
  }

  void push_scope() {
    m_scopes.emplace_back(std::make_shared<Scope>(m_scopes.back()));
  }
  void pop_scope() noexcept {
    assert(m_scopes.size() > 1 && "Cannot pop scope, already at global scope!");
    m_scopes.pop_back();
  }

  bool is_present(BindingId id) { return m_scopes.back()->is_present(id); }

  void define(BindingId id) { m_scopes.back()->define(id); }

private:
  std::vector<std::shared_ptr<Scope>> m_scopes;
};

using BoundVariableScopeGuard = core::ScopeGuard<BoundVariableEnvironment>;

struct FreeVariableCollector {

  using FreeVariables = std::unordered_set<IdentifierExpr>;

  explicit FreeVariableCollector(
      Context &ctx, const std::vector<IdentifierExpr> &initial_parameters)
      : m_ctx(ctx) {
    for (const auto &parameter : initial_parameters) {
      m_env.define(parameter.m_id);
    }
  }

  static void drain(FreeVariables &into, FreeVariables &from) {
    into.insert(std::make_move_iterator(from.begin()),
                std::make_move_iterator(from.end()));
    from.clear();
  }

  void collect_append(const auto &to_collect, FreeVariables &into) {
    FreeVariables new_free_variables;
    new_free_variables = collect(to_collect);
    drain(into, new_free_variables);
  }

  FreeVariables collect(const Block &block) {
    BoundVariableScopeGuard guard{m_env};
    FreeVariables free_variables;
    for (const auto &statement : block.m_statements) {
      auto new_free_variables = collect(statement);
      drain(free_variables, new_free_variables);
    }

    if (!block.m_final_expression.has_value()) {
      return free_variables;
    }

    auto new_free_variables = collect(block.m_final_expression.value());
    drain(free_variables, new_free_variables);

    return free_variables;
  }

  FreeVariables collect(const Statement &statement) {
    return std::visit(*this, statement);
  }

  FreeVariables operator()(const LetBinding &let_binding) {
    // Bind this identifier to the env
    m_env.define(let_binding.m_identifier);

    FreeVariables free_variables;
    collect_append(let_binding.m_expression, free_variables);
    return free_variables;
  }

  FreeVariables operator()(const ExpressionStatement &expression_statement) {
    return collect(expression_statement.m_expression);
  }

  FreeVariables collect(ExprId expr_id) {
    return std::visit(*this, m_ctx.m_arena.get(expr_id).m_expr_kind);
  }

  FreeVariables operator()(const IdentifierExpr &identifier) {
    // We've found a binding, check if in scope
    if (m_env.is_present(identifier.m_id)) {
      // Already bound, nothing to do
      return {};
    }
    // Free variable found
    return {identifier};
  }

  FreeVariables operator()(const Unit & /* unused */) { return {}; }
  FreeVariables operator()(const I8 & /* unused */) { return {}; }
  FreeVariables operator()(const I32 & /* unused */) { return {}; }
  FreeVariables operator()(const I64 & /* unused */) { return {}; }
  FreeVariables operator()(const Bool & /* unused */) { return {}; }
  FreeVariables operator()(const Char & /* unused */) { return {}; }

  FreeVariables operator()(const Block &block) { return collect(block); }

  FreeVariables operator()(const IfExpr &if_expr) {
    FreeVariables free_variables;
    collect_append(if_expr.m_condition, free_variables);
    collect_append(if_expr.m_then_block, free_variables);
    if (if_expr.m_else_block.has_value()) {
      collect_append(if_expr.m_else_block.value(), free_variables);
    }

    return free_variables;
  }

  FreeVariables operator()(const CallExpr &call_expr) {
    FreeVariables free_variables;
    collect_append(call_expr.m_callee, free_variables);
    for (const auto &argument : call_expr.m_arguments) {
      collect_append(argument, free_variables);
    }
    return free_variables;
  }

  FreeVariables operator()(const BinaryExpr &binary_expr) {
    FreeVariables free_variables;
    collect_append(binary_expr.m_lhs, free_variables);
    collect_append(binary_expr.m_rhs, free_variables);
    return free_variables;
  }

  FreeVariables operator()(const UnaryExpr &unary_expr) {
    FreeVariables free_variables;
    collect_append(unary_expr.m_expression, free_variables);
    return free_variables;
  }

  FreeVariables operator()(const ReturnExpr &return_expr) {
    FreeVariables free_variables;
    collect_append(return_expr.m_expression, free_variables);
    return free_variables;
  }

  FreeVariables operator()(const CastExpr &cast_expr) {
    FreeVariables free_variables;
    collect_append(cast_expr.m_expression, free_variables);
    return free_variables;
  }

  FreeVariables operator()(const LambdaExpr &lambda_expr) {
    BoundVariableScopeGuard guard{m_env};
    FreeVariables free_variables;

    // Need to mark the parameters as bound in the body
    for (const auto &parameter : lambda_expr.m_parameters) {
      m_env.define(parameter.m_id);
    }

    // Now we recurse into the body
    collect_append(lambda_expr.m_body, free_variables);

    return free_variables;
  }

  Context &m_ctx;
  BoundVariableEnvironment m_env;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
