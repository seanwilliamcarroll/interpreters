//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Converts AST type identifiers to HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/nodes.hpp>
#include <ast/types.hpp>
#include <exceptions.hpp>
#include <hir/context.hpp>
#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <memory>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TypeConverter {

  TypeId convert(const ast::TypeIdentifier &type) {
    return std::visit(*this, type);
  }

  TypeId operator()(const ast::DefinedType & /*type*/) {
    throw core::InternalCompilerError("user-defined types not yet implemented");
  }

  TypeId operator()(const ast::PrimitiveTypeIdentifier &type) {
    return m_ctx.m_type_registry.intern(PrimitiveTypeValue{type.m_type});
  }

  TypeId operator()(const std::unique_ptr<ast::FunctionTypeIdentifier> &type) {
    std::vector<TypeId> param_types;
    param_types.reserve(type->m_parameter_types.size());
    for (const auto &param : type->m_parameter_types) {
      param_types.push_back(convert(param));
    }
    auto return_type = convert(type->m_return_type);
    return m_ctx.m_type_registry.intern(
        FunctionType{std::move(param_types), return_type});
  }

  TypeId get_type(const ast::TypeIdentifier &identifier) {
    return std::visit((*this), identifier);
  }

  TypeId get_type(const ast::Identifier &identifier) {
    if (identifier.m_type.has_value()) {
      return std::visit((*this), identifier.m_type.value());
    }
    return m_ctx.m_type_unifier.new_type_var();
  }

  Identifier convert_parameter(const ast::Identifier &identifier) {
    return {{identifier.m_location}, identifier.m_name, get_type(identifier)};
  }

  std::pair<std::vector<Identifier>, std::vector<TypeId>>
  convert_parameters(const std::vector<ast::Identifier> &ast_params) {
    std::vector<Identifier> parameters;
    std::vector<TypeId> parameter_types;
    parameters.reserve(ast_params.size());
    parameter_types.reserve(ast_params.size());
    for (const auto &param : ast_params) {
      parameters.emplace_back(convert_parameter(param));
      parameter_types.emplace_back(parameters.back().m_type);
    }
    return {std::move(parameters), std::move(parameter_types)};
  }

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
