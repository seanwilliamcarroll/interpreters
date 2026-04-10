//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Statement and block checker — scope management, block checking,
//*            let bindings.
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

struct StatementChecker {
  Statement operator()(const ast::LetBinding &);

  Statement operator()(const ast::Expression &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
