//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Function declaration for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>

#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct FunctionDeclaration {
  Value m_function_id;
  TypeId m_return_type;
  std::vector<Parameter> m_parameters;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
