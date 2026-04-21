//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : LLVM IR syntax tokens and keywords used by the formatter.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string_view>

//****************************************************************************
namespace bust::codegen::ir_syntax {
//****************************************************************************

// Module / function directives
constexpr std::string_view declare = "declare";
constexpr std::string_view define = "define";
constexpr std::string_view constant = "constant";
constexpr std::string_view type_keyword = "type";

// Instruction opcodes
constexpr std::string_view load = "load";
constexpr std::string_view store = "store";
constexpr std::string_view alloca_op = "alloca";
constexpr std::string_view getelementptr = "getelementptr";
constexpr std::string_view ptrtoint = "ptrtoint";
constexpr std::string_view icmp = "icmp";
constexpr std::string_view call = "call";
constexpr std::string_view call_void = "call void";
constexpr std::string_view br = "br";
constexpr std::string_view ret = "ret";
constexpr std::string_view ret_void = "ret void";
constexpr std::string_view sub = "sub";
constexpr std::string_view xor_op = "xor";

// Auxiliary keywords appearing in instruction syntax
constexpr std::string_view label = "label";
constexpr std::string_view to = "to";

// Type spellings used literally by the formatter (duplicated from
// TypeArena::to_string — ok for now, these are stable IR types).
constexpr std::string_view ptr = "ptr";
constexpr std::string_view i1 = "i1";

constexpr std::string_view percent_symbol = "%";

//****************************************************************************
} // namespace bust::codegen::ir_syntax
//****************************************************************************
