//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Visitor that clones an HIR statement while substituting
//*            type variables according to a TypeSubstitution.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/nodes.hpp"
#include "mono/context.hpp"
#include "mono/expression_substituter.hpp"
#include "mono/let_binding_substituter.hpp"

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct StatementSubstituter {
  hir::Statement substitute(const hir::Statement &statement) {
    return std::visit(*this, statement);
  }

  hir::Statement operator()(const hir::Expression &expression) {
    return ExpressionSubstituter{m_ctx}.substitute(expression);
  }

  hir::Statement operator()(const hir::LetBinding &let_binding) {
    return LetBindingSubstituter{m_ctx}.substitute(let_binding);
  }

  SubstitutionContext &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
