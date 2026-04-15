//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Monomorphization pass — specializes polymorphic let bindings
//*            into concrete type-specialized copies based on the
//*            instantiation records attached to the program.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Monomorphization pass. Consumes the instantiation records attached to
/// the program and emits a new program in which every polymorphic let
/// binding has been replaced by one or more concrete specializations.
struct Monomorpher {
  hir::Program operator()(hir::Program program);
};

//****************************************************************************
} // namespace bust
//****************************************************************************
