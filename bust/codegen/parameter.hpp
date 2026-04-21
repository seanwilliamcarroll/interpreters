//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
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
  std::string m_name;
  TypeId m_type;
};

struct Argument {
  Handle m_name;
  TypeId m_type;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
