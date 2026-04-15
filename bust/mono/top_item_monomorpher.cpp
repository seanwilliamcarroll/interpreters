//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the top-level item monomorphization
//*            walker.
//*
//*
//****************************************************************************

#include "mono/top_item_monomorpher.hpp"
#include "hir/nodes.hpp"
#include "mono/expression_substituter.hpp"
#include "mono/let_binding_monomorpher.hpp"
#include <iterator>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

std::vector<hir::TopItem>
TopItemMonomorpher::monomorph(const hir::TopItem &top_item) {
  return std::visit(*this, top_item);
}

std::vector<hir::TopItem>
TopItemMonomorpher::operator()(const hir::LetBinding &let_binding) {

  auto let_bindings = LetBindingMonomorpher{m_ctx}.monomorph(let_binding, {});

  std::vector<hir::TopItem> top_items;
  top_items.insert(top_items.end(),
                   std::make_move_iterator(let_bindings.begin()),
                   std::make_move_iterator(let_bindings.end()));

  return top_items;
}

std::vector<hir::TopItem>
TopItemMonomorpher::operator()(const hir::FunctionDef &function_def) {
  // At this time, all top level functions require annotation, so no
  // polymorphism to handle here, just recurse on down

  auto sub_context = SubstitutionContext{m_ctx, {}};
  auto new_body =
      ExpressionSubstituter{sub_context}.substitute(function_def.m_body);

  std::vector<hir::TopItem> top_items;
  top_items.emplace_back(hir::FunctionDef{{function_def.m_location},
                                          function_def.m_signature,
                                          std::move(new_body)});

  return top_items;
}

std::vector<hir::TopItem> TopItemMonomorpher::operator()(
    const hir::ExternFunctionDeclaration &extern_func_declaration) {
  std::vector<hir::TopItem> top_items;
  top_items.emplace_back(
      hir::ExternFunctionDeclaration{{extern_func_declaration.m_location},
                                     extern_func_declaration.m_signature});
  return top_items;
}

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
