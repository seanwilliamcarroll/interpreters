//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Let binding lowerer — HIR let bindings to ZIR.
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

struct LetBindingLowerer {

  LetBinding lower(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
