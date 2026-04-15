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
    auto substituter = ExpressionSubstituter{m_ctx};

    auto new_identifier = substituter.substitute(let_binding.m_variable);

    auto expression = substituter.substitute(let_binding.m_expression);

    return {{let_binding.m_location},
            std::move(new_identifier),
            std::move(expression)};
  }

  SubstitutionContext &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
