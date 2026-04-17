//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Free type variable collection visitor for bust HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/context.hpp>
#include <hir/types.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct FreeTypeVariableCollector {

  explicit FreeTypeVariableCollector(Context &ctx) : m_ctx(ctx) {}

  void collect(const TypeKind &type) { std::visit(*this, type); }
  void collect(const TypeId &type) { collect(m_ctx.m_type_registry.get(type)); }

  void operator()(const PrimitiveTypeValue & /*unused*/) {}

  void operator()(const TypeVariable &type) {
    // Let's try to resolve the types first
    auto resolved_type_id = m_ctx.m_type_unifier.find(type);
    if (!m_ctx.is_type_variable(resolved_type_id)) {
      return;
    }
    m_free_type_variables.emplace_back(resolved_type_id);
  }

  void operator()(const FunctionType &type) {
    for (const auto &parameter : type.m_parameters) {
      collect(parameter);
    }

    collect(type.m_return_type);
  }

  void operator()(const NeverType & /*unused*/) {}

  std::vector<TypeId> &free_type_variables() { return m_free_type_variables; }

private:
  hir::Context &m_ctx;
  std::vector<TypeId> m_free_type_variables;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
