//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Typed handle referring to a Function. Constructed only by
//*            IRBuilder; safe to store in instructions as a call target.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/value.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Function;

struct FunctionHandle {
  [[nodiscard]] const GlobalHandle &name() const;

private:
  explicit FunctionHandle(Function *function) : m_function(function) {}

  Function *m_function;

  friend struct IRBuilder;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
