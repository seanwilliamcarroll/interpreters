//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Runtime value type for blip evaluation
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <string>
#include <variant>

//****************************************************************************
namespace blip {
//****************************************************************************

struct Unit {};

using Value = std::variant<int, double, bool, std::string, Unit>;

std::string value_to_string(const Value &value);

//****************************************************************************
} // namespace blip
//****************************************************************************
