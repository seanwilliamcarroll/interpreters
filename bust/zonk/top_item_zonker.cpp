//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item zonker implementation.
//*
//*
//****************************************************************************

#include <zonk/top_item_zonker.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

hir::TopItem TopItemZonker::operator()(hir::FunctionDef function_def) {
  // TODO: Deep-resolve all TypeIds in the function definition,
  // replacing any that point to TypeVariables with their concrete types.
  return function_def;
}

hir::TopItem TopItemZonker::operator()(hir::LetBinding let_binding) {
  // TODO: Deep-resolve all TypeIds in the let binding.
  return let_binding;
}

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
