//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Converts AST type identifiers to HIR types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "ast/types.hpp"
#include "exceptions.hpp"
#include "hir/types.hpp"
#include <memory>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************

struct TypeConverter {
  hir::Type operator()(const ast::DefinedType &type) {
    throw core::CompilerException(
        "TypeChecker", std::string("UNIMPLEMENTED") + " " + __PRETTY_FUNCTION__,
        type.m_location);
  }

  hir::Type operator()(const ast::PrimitiveTypeIdentifier &type) {
    return hir::PrimitiveTypeValue{{type.m_location}, type.m_type};
  }

  hir::Type
  operator()(const std::unique_ptr<ast::FunctionTypeIdentifier> &type) {
    std::vector<hir::Type> param_types;
    param_types.reserve(type->m_parameter_types.size());
    for (const auto &param : type->m_parameter_types) {
      param_types.push_back(std::visit(*this, param));
    }
    auto return_type = std::visit(*this, type->m_return_type);
    return std::make_unique<hir::FunctionType>(hir::FunctionType{
        {type->m_location}, std::move(param_types), std::move(return_type)});
  }
};

//****************************************************************************
} // namespace bust
//****************************************************************************
