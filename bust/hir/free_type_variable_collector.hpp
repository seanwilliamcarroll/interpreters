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

  void collect(const TypeKind &type) { std::visit(*this, type); }
  void collect(const TypeId &type) { collect(m_ctx.m_type_registry.get(type)); }

  void operator()(const PrimitiveTypeValue &) {}

  void operator()(const TypeVariable &type) {
    // Let's try to resolve the types first
    auto resolved_type_id = m_ctx.m_type_unifier.find(type);
    if (std::holds_alternative<TypeVariable>(
            m_ctx.m_type_registry.get(resolved_type_id))) {
      m_free_type_variables.emplace_back(type);
    }
  }

  void operator()(const FunctionType &type) {
    for (const auto &parameter : type.m_parameters) {
      collect(m_ctx.m_type_registry.get(parameter));
    }

    collect(m_ctx.m_type_registry.get(type.m_return_type));
  }

  void operator()(const NeverType &) {}

  hir::Context &m_ctx;
  std::vector<TypeVariable> m_free_type_variables{};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
