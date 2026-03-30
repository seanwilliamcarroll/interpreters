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

  SourceLocation(const char *file_name = "", unsigned int line = 1,
                 unsigned int column = 0)
      : line(line), column(column), file_name(file_name) {}

  bool operator==(const SourceLocation &) const = default;

  const unsigned int line;
  const unsigned int column;
  const std::string file_name;
};

inline std::ostream &operator<<(std::ostream &out, const SourceLocation &loc) {
  return out << "SourceLocation: " << loc.file_name << "::" << loc.line << ":"
             << loc.column;
}

//****************************************************************************
} // namespace core
//****************************************************************************
