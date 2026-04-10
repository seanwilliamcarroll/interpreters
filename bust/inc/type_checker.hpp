//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Type checking pass for bust programs.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/nodes.hpp>
#include <hir/environment.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Type checking pass. Validates that all expressions have consistent types.
/// Currently a pass-through — will eventually produce a typed AST.
struct TypeChecker {
  hir::Program operator()(const ast::Program &program);

private:
  hir::Environment m_env;
};

//****************************************************************************
} // namespace bust
//****************************************************************************
