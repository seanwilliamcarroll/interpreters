//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of the top-level item lowerer.
//*
//*
//****************************************************************************

#include <zir/top_item_lowerer.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

TopItem TopItemLowerer::lower(const hir::TopItem &top_item) {
  return std::visit(*this, top_item);
}

TopItem TopItemLowerer::TopItemLowerer::operator()(const hir::FunctionDef &) {
  return {};
}

TopItem TopItemLowerer::TopItemLowerer::operator()(
    const hir::ExternFunctionDeclaration &) {
  return {};
}

TopItem TopItemLowerer::operator()(const hir::LetBinding &) { return {}; }

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
