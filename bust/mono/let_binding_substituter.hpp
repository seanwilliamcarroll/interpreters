//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Visitor that clones an HIR let binding while substituting
//*            type variables according to a TypeSubstitution.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "exceptions.hpp"
#include "hir/nodes.hpp"
#include "mono/context.hpp"
#include "mono/expression_substituter.hpp"
#include "mono/name_mangler.hpp"

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct LetBindingSubstituter {
  hir::LetBinding substitute(const hir::LetBinding &let_binding) {
    // Need to check if this is a callable

    auto expression =
        ExpressionSubstituter{m_ctx}.substitute(let_binding.m_expression);

    // // Bound to a function, we need to monomorphize
    // auto new_name = Mangler{m_ctx.type_registry()}.mangle(
    //     let_binding.m_variable.m_name, let_binding.m_variable.m_id,
    //     expression.m_type);

    // auto new_identifier = hir::Identifier {
    //   {let_binding.m_variable.m_location}, std::move(new_name),
    // };

    return {};
  }

  SubstitutionContext &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
