//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type representation for the blip type checker
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string>

//****************************************************************************
namespace blip {
//****************************************************************************

enum class Type : uint8_t {
  Int,
  Double,
  Bool,
  String,
  Unit,
  Fn,
};

std::string type_to_string(Type type);

Type string_to_type(const std::string &name);

//****************************************************************************
} // namespace blip
//****************************************************************************
