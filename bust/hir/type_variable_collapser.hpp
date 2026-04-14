//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Collapse unified type variables to their root representative.
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

struct TypeVariableCollapser {

  TypeId collapse(const TypeKind &type) { return std::visit(*this, type); }
  TypeId collapse(const TypeId &type) {
    return collapse(m_ctx.m_type_registry.get(type));
  }

  TypeId operator()(const PrimitiveTypeValue &type) {
    return m_ctx.m_type_registry.intern(type);
  }

  TypeId operator()(const TypeVariable &type) {
    auto resolved_type_id = m_ctx.m_type_unifier.find(type);
    return resolved_type_id;
  }

  TypeId operator()(const FunctionType &type) {
    std::vector<TypeId> parameters;
    parameters.reserve(type.m_parameters.size());
    for (const auto &parameter : type.m_parameters) {
      parameters.push_back(collapse(parameter));
    }

    auto function_type =
        FunctionType{.m_parameters = std::move(parameters),
                     .m_return_type = collapse(type.m_return_type)};
    return m_ctx.m_type_registry.intern(function_type);
  }

  TypeId operator()(const NeverType &) { return m_ctx.m_type_registry.m_never; }

  hir::Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
