//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Purpose : SourceLocation class
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ostream>
#include <sstream>
#include <string>

//****************************************************************************
namespace core {
//****************************************************************************

struct SourceLocation {

  bool operator==(const SourceLocation &) const = default;

  const char *const file_name{};
  const unsigned int line{1};
  const unsigned int column{0};
};

inline std::ostream &operator<<(std::ostream &out, const SourceLocation &loc) {
  return out << "SourceLocation: " << loc.file_name << "::" << loc.line << ":"
             << loc.column;
}

inline std::string to_string(const SourceLocation &loc) {
  std::ostringstream out;
  out << loc;
  return out.str();
}

inline std::string operator+(const std::string &lhs,
                             const SourceLocation &loc) {
  return lhs + to_string(loc);
}

inline std::string operator+(const SourceLocation &loc,
                             const std::string &rhs) {
  return to_string(loc) + rhs;
}

struct HasLocation {
  SourceLocation m_location;
};

//****************************************************************************
} // namespace core
//****************************************************************************
