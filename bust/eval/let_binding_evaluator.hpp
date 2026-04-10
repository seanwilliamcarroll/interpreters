//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Let-binding evaluator for bust tree-walking evaluator.
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

struct LetBindingEvaluator {

  Value operator()(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
