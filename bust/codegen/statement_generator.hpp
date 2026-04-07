//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Statement generator for bust LLVM IR codegen.
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

struct StatementGenerator {
  void operator()(const hir::Expression &);
  void operator()(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
