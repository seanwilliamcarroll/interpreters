//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR program — top-level container holding the arena and
//*            the list of top-level items.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <zir/arena.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Program {
  Arena m_arena;
  std::vector<TopItem> m_top_items;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
