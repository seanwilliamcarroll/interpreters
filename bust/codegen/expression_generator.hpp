//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Expression generator for bust LLVM IR codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/context.hpp"
#include "hir/nodes.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct ExpressionGenerator {
  void operator()(const hir::Expression &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
