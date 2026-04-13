//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Let binding zonker implementation.
//*
//*
//****************************************************************************

#include <zonk/let_binding_zonker.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

hir::LetBinding LetBindingZonker::zonk(hir::LetBinding let_binding) {
  // TODO: Zonk the variable's type and the expression
  return let_binding;
}

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
