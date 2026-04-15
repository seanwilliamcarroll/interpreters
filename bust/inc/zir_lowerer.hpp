//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR lowering pass — converts HIR (with resolved unifier
//*            state) into the arena-backed ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

/// ZIR lowering pass. Consumes HIR + unifier state and produces a ZIR
/// program whose types contain no type variables and whose expressions
/// live in a flat arena.
struct ZirLowerer {
  zir::Program operator()(hir::Program program);
};

//****************************************************************************
} // namespace bust
//****************************************************************************
