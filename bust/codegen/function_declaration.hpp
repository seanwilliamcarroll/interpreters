//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Function declaration for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/handle.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct FunctionDeclaration {
  Handle m_function_id;
  LLVMType m_return_type;
  std::vector<Parameter> m_parameters;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
