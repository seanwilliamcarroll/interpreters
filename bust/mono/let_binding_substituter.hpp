//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Visitor that clones an HIR let binding while substituting
//*            type variables according to a TypeSubstitution.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/nodes.hpp"
#include "mono/context.hpp"

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct LetBindingSubstituter {

  hir::LetBinding substitute(const hir::LetBinding & // let_binding
  ) {
    // Need to check if this is a callable

    return {};
  }

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
