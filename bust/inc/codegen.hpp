//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : LLVM IR code generation pass for bust ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <zir/program.hpp>

#include <string>

//****************************************************************************
namespace bust {
//****************************************************************************

/// Lowers a typed, monomorphed bust program (ZIR) to textual LLVM IR.
/// Follows the pass-functor convention so it slots into run_pipeline.
struct CodeGen {
  std::string operator()(const zir::Program &program);
};

//****************************************************************************
} // namespace bust
//****************************************************************************
