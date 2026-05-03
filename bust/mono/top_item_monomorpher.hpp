//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Walker that monomorphizes each top-level item, producing
//*            a new TopItem (or a fan-out of specialized top-level
//*            bindings when the item is a polymorphic let).
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <vector>

//****************************************************************************
#include <hir/nodes.hpp>
#include <mono/context.hpp>

namespace bust::mono {
//****************************************************************************

struct TopItemMonomorpher {

  std::vector<hir::TopItem> monomorph(const hir::TopItem &);

  std::vector<hir::TopItem> operator()(const hir::FunctionDef &);
  std::vector<hir::TopItem> operator()(const hir::ExternFunctionDeclaration &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
