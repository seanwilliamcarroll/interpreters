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

#include <sstream>
#include <stdexcept>
#include <string_view>

#include <source_location.hpp>

//****************************************************************************
namespace core {
//****************************************************************************

struct CompilerException : std::runtime_error {
  CompilerException(const char *phase, std::string_view message,
                    const SourceLocation &loc)
      : std::runtime_error([&] {
          std::stringstream s;
          s << phase << ": " << loc << " " << message;
          return s.str();
        }()) {}
};

//****************************************************************************
} // namespace core
//****************************************************************************
