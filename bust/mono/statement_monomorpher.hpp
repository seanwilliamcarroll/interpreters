//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement-level monomorpher. Dispatches on the statement
//*            variant: expressions are cloned 1:1 via the expression
//*            substituter, while let bindings are handed off to the
//*            let binding monomorpher which may fan out into multiple
//*            specialized bindings.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/nodes.hpp"
#include "mono/context.hpp"
#include "mono/let_binding_monomorpher.hpp"
#include <vector>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct StatementMonomorpher {

  std::vector<hir::Statement> monomorph(const hir::Statement &);

  // std::vector<hir::Statement> operator()(const hir::Expression &expression) {

  // }

  // std::vector<hir::Statement> operator()(const hir::LetBinding &let_binding)
  // {
  //   auto new_let_bindings =
  //   LetBindingMonomorpher{m_ctx}.monomorph(let_binding);

  //   // Feels silly to have to do this, but the types require it
  //   std::vector<hir::Statement> statements;
  //   statements.reserve(new_let_bindings.size());
  //   for (auto &new_let_binding : new_let_bindings) {
  //     statements.emplace_back(std::move(new_let_binding));
  //   }
  //   return statements;
  // }

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
