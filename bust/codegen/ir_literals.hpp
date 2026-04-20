//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Textual spellings of LLVM IR constants used throughout codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string_view>

//****************************************************************************
namespace bust::codegen::ir_literals {
//****************************************************************************

constexpr std::string_view null = "null";
constexpr std::string_view true_ = "true";
constexpr std::string_view false_ = "false";
constexpr std::string_view zero = "0";
constexpr std::string_view one = "1";
constexpr std::string_view all_ones = "-1";

//****************************************************************************
} // namespace bust::codegen::ir_literals
//****************************************************************************
