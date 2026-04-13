//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item evaluator for bust tree-walking evaluator.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <eval/context.hpp>
#include <eval/values.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct TopItemEvaluator {
  Value operator()(const hir::FunctionDef &);
  Value operator()(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
