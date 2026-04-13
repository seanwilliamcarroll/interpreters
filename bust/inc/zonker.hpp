//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Zonking pass — resolves all type variables in the HIR
//*            to their concrete types after type checking.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Zonking pass. Consumes the unifier state attached to the program and
/// deep-resolves every TypeId, guaranteeing no TypeVariables remain in
/// the output.
struct Zonker {
  hir::Program operator()(hir::Program program);
};

//****************************************************************************
} // namespace bust
//****************************************************************************
