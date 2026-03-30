//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Exception classes
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <stdexcept>
#include <string_view>

#include <sc/sc.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

struct CompilerException : std::runtime_error {
  CompilerException(const char *phase, std::string_view message,
                    const SourceLocation &);
};

//****************************************************************************
} // namespace sc
//****************************************************************************
