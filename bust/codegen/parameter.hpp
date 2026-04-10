//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Parameter type for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/handle.hpp>
#include <codegen/types.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Parameter {
  ParameterHandle m_name;
  LLVMType m_type;
};

struct Argument {
  Handle m_name;
  LLVMType m_type;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
