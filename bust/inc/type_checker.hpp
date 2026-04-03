//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type checking pass for bust programs.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast.hpp>
#include <hir.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Type checking pass. Validates that all expressions have consistent types.
/// Currently a pass-through — will eventually produce a typed AST.
struct TypeChecker {
  Program operator()(Program program) const;
};

//****************************************************************************
} // namespace bust
//****************************************************************************
