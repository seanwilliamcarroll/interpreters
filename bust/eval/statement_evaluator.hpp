//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Statement evaluator for bust tree-walking evaluator.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "eval/context.hpp"
#include "eval/values.hpp"
#include "hir/nodes.hpp"

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct StatementEvaluator {

  Value operator()(const hir::Expression &);
  Value operator()(const hir::LetBinding &);

  Context m_ctx;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
