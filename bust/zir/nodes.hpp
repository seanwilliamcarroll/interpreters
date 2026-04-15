//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR expression and program nodes. Arena-backed: all
//*            cross-references are ExprId handles, no unique_ptr trees.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <zir/types.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

// Program, ExprArena, ExprId, ExprKind variants — to be designed.
struct Program {};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
