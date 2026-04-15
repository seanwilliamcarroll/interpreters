//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Monomorpher that turns a single polymorphic let binding
//*            into N specialized let bindings by consulting the
//*            instantiation records and invoking the let binding
//*            substituter once per record.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/instantiation_record.hpp"
#include "hir/nodes.hpp"
#include "mono/context.hpp"

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct LetBindingMonomorpher {

  std::vector<hir::LetBinding> monomorph(const hir::LetBinding &,
                                         const hir::TypeSubstitution &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
