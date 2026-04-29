//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the Monomorpher pipeline pass.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/type_arena.hpp>
#include <hir/unifier_state.hpp>
#include <mono/context.hpp>
#include <mono/nodes.hpp>
#include <mono/top_item_monomorpher.hpp>
#include <monomorpher.hpp>
#include <source_location.hpp>

#include <iterator>
#include <utility>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************

mono::Program Monomorpher::operator()(hir::Program program) {
  // We have a program and within it are instantiation records
  auto context = mono::Context(
      program.m_type_arena, std::move(program.m_unifier_state),
      program.m_instantiation_records, program.m_next_let_binding_id);

  std::vector<hir::TopItem> top_items;
  for (const auto &top_item : program.m_top_items) {
    auto new_top_items = mono::TopItemMonomorpher{context}.monomorph(top_item);
    top_items.insert(top_items.end(),
                     std::make_move_iterator(new_top_items.begin()),
                     std::make_move_iterator(new_top_items.end()));
    new_top_items.clear();
  }

  return {{program.m_location},
          std::move(program.m_type_arena),
          std::move(top_items),
          std::move(program.m_unifier_state)};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
