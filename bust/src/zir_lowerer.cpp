//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the ZirLowerer pipeline pass.
//*
//*
//****************************************************************************

#include "exceptions.hpp"
#include "zir/context.hpp"
#include "zir/nodes.hpp"
#include "zir/top_item_lowerer.hpp"
#include "zir/type_resolver.hpp"
#include <zir_lowerer.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

zir::Program ZirLowerer::operator()(hir::Program program) {
  auto type_registry = std::move(program.m_type_registry);

  if (!program.m_unifier_state.has_value()) {
    throw core::InternalCompilerError("UniferState required for ZIR lowering!");
  }

  auto unifier_state = std::move(program.m_unifier_state.value());

  auto resolver = zir::TypeResolver{type_registry, std::move(unifier_state)};

  auto context = zir::Context{type_registry, std::move(resolver)};

  std::vector<zir::TopItem> new_top_items;
  new_top_items.reserve(program.m_top_items.size());
  for (const auto &top_item : program.m_top_items) {
    new_top_items.emplace_back(zir::TopItemLowerer{context}.lower(top_item));
  }

  return zir::Program{.m_arena = std::move(context.m_arena),
                      .m_top_items = std::move(new_top_items)};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
