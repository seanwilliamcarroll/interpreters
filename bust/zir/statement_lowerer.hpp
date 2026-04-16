//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement lowerer — HIR statements to ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "zir/context.hpp"
#include <hir/nodes.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct StatementLowerer {
  // Interface to be designed.

  Statement lower(const hir::Statement &);

  Statement operator()(const hir::Expression &);

  Statement operator()(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
