//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Purpose : Exception classes
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
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

template <typename Fn>
auto promote_to_compiler_exception(const char *phase,
                                   const SourceLocation &location, Fn &&func) {
  try {
    return func();
  } catch (std::runtime_error &error) {
    throw CompilerException(phase, error.what(), location);
  }
}

struct InternalCompilerError : std::logic_error {
  InternalCompilerError(
      std::string_view message,
      std::source_location loc = std::source_location::current())
      : std::logic_error(
            std::string(loc.file_name()) + ":" + std::to_string(loc.line()) +
            " in " + loc.function_name() + ": ICE: " + std::string(message)) {}
};

//****************************************************************************
} // namespace core
//****************************************************************************
