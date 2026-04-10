//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Top-level item checker — handles FunctionDef and top-level
//*            LetBinding.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/nodes.hpp>
#include <hir/context.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TopItemChecker {
  void collect_function_signature(const ast::FunctionDef &);
  TopItem operator()(const ast::FunctionDef &);
  TopItem operator()(const ast::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
