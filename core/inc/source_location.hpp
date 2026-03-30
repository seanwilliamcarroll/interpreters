//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : SourceLocation class
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ostream>
#include <string>

//****************************************************************************
namespace core {
//****************************************************************************

struct SourceLocation {

  bool operator==(const SourceLocation &) const = default;

  const std::string file_name{};
  const unsigned int line{1};
  const unsigned int column{0};
};

inline std::ostream &operator<<(std::ostream &out, const SourceLocation &loc) {
  return out << "SourceLocation: " << loc.file_name << "::" << loc.line << ":"
             << loc.column;
}

//****************************************************************************
} // namespace core
//****************************************************************************
