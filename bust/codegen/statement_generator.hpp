//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement generator for bust LLVM IR codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/context.hpp>
#include <codegen/value.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct StatementGenerator {
  Value generate(const zir::Statement &);

  Value operator()(const zir::ExpressionStatement &);
  Value operator()(const zir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
