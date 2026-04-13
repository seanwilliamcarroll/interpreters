//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the Zonker pipeline pass.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <hir/nodes.hpp>
#include <variant>
#include <vector>
#include <zonk/top_item_zonker.hpp>
#include <zonker.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

hir::Program Zonker::operator()(hir::Program program) {
  if (!program.m_unifier_state.has_value()) {
    throw core::InternalCompilerError(
        "zonker invoked on program without unifier state");
  }

  auto ctx = zonk::Context{
      program.m_type_registry,
      zonk::TypeResolver{program.m_type_registry,
                         std::move(program.m_unifier_state.value())}};

  std::vector<hir::TopItem> zonked_items;
  zonked_items.reserve(program.m_top_items.size());
  for (auto &top_item : program.m_top_items) {
    zonked_items.push_back(
        std::visit(zonk::TopItemZonker{ctx}, std::move(top_item)));
  }

  return {{program.m_location},
          std::move(ctx.m_new_type_registry),
          std::move(zonked_items),
          {}};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
