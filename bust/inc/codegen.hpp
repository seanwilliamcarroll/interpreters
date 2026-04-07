//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LLVM IR code generation pass for bust HIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <string>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Lowers a typed bust program (HIR) to textual LLVM IR.
/// Follows the pass-functor convention so it slots into run_pipeline.
struct CodeGen {
  std::string operator()(const hir::Program &program);
};

//****************************************************************************
} // namespace bust
//****************************************************************************
