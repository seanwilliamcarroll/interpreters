//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the evaluator pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/type_registry.hpp"
#include <eval/environment.hpp>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct Context {
  Environment m_env{};
  const hir::TypeRegistry &m_type_registry;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
