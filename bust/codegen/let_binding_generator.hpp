//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Let-binding generator for bust LLVM IR codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/context.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct LetBindingGenerator {
  void generate(const zir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
