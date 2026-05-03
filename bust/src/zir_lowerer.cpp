//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the ZirLowerer pipeline pass.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/type_arena.hpp>
#include <hir/unifier_state.hpp>
#include <mono/nodes.hpp>
#include <zir/arena.hpp>
#include <zir/context.hpp>
#include <zir/nodes.hpp>
#include <zir/program.hpp>
#include <zir/top_item_lowerer.hpp>
#include <zir_lowerer.hpp>

#include <utility>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************

zir::Program ZirLowerer::operator()(mono::Program program) {
  auto type_arena = std::move(program.m_type_arena);

  auto unifier_state = std::move(program.m_unifier_state);

  auto context = zir::Context(type_arena, std::move(unifier_state));

  // Collect global bindings into context
  auto collector = zir::TopItemCollector{.m_ctx = context};
  for (const auto &top_item : program.m_top_items) {
    collector.collect(top_item);
  }

  std::vector<zir::TopItem> new_top_items;
  new_top_items.reserve(program.m_top_items.size());
  for (const auto &top_item : program.m_top_items) {
    new_top_items.emplace_back(zir::TopItemLowerer{context}.lower(top_item));
  }

  return zir::Program{.m_arena = std::move(context.arena()),
                      .m_top_items = std::move(new_top_items)};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
