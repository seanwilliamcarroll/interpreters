//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Out-of-line definitions for FunctionHandle.
//*
//*
//****************************************************************************

#include <codegen/function.hpp>
#include <codegen/function_handle.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

const Handle &FunctionHandle::name() const {
  return m_function->signature().m_function_id;
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
