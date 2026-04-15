//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the top-level item monomorphization
//*            walker.
//*
//*
//****************************************************************************

#include "mono/top_item_monomorpher.hpp"
#include "hir/nodes.hpp"

//****************************************************************************
namespace bust::mono {
//****************************************************************************

std::vector<hir::TopItem>
TopItemMonomorpher::monomorph(const hir::TopItem &top_item) {
  return std::visit(*this, top_item);
}

std::vector<hir::TopItem>
TopItemMonomorpher::operator()(const hir::LetBinding &) {
  return {};
}

std::vector<hir::TopItem>
TopItemMonomorpher::operator()(const hir::FunctionDef &) {
  return {};
}

std::vector<hir::TopItem>
TopItemMonomorpher::operator()(const hir::ExternFunctionDeclaration &) {
  return {};
}

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
