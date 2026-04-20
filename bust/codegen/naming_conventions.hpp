//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Centralized naming conventions for codegen IR emission.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************
namespace conventions {

constexpr std::string closure_type = "closure";

inline std::string make_closure_name(const std::string &input) {
  return input + "." + closure_type;
}

constexpr std::string thunk_suffix = "thunk";

inline std::string make_thunk(const std::string &input) {
  return input + "." + thunk_suffix;
}

} // namespace conventions
//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
