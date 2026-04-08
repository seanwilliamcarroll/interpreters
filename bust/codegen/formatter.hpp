//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Formatter that serializes the in-memory IR to LLVM IR text.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/basic_block.hpp"
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Formatter {
  std::string operator()(const Function &function);
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
