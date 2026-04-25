//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Parameter type for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/types.hpp>
#include <codegen/value.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Parameter {
  std::string m_name;
  TypeId m_type;
};

struct Index {
  size_t m_index;
  TypeId m_type;
};

struct Argument {
  Handle m_name;
  TypeId m_type;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
