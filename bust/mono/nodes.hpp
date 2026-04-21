//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Monomorphed AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

// --- Program ---------------------------------------------------------------

struct Program : public core::HasLocation {
  hir::TypeArena m_type_arena;
  std::vector<hir::TopItem> m_top_items;
  hir::UnifierState m_unifier_state;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
