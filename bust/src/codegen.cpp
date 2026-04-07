//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LLVM IR code generation pass implementation.
//*
//*
//****************************************************************************

#include <codegen.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

std::string CodeGen::operator()(const hir::Program & /*program*/) {
  // Stub: ignore the program and emit a trivial main that returns 0.
  // Real lowering is added feature-by-feature, test-first.
  return "define i64 @main() {\n"
         "  ret i64 0\n"
         "}\n";
}

//****************************************************************************
} // namespace bust
//****************************************************************************
