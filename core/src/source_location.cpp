//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : SourceLocation class source file
//*
//*
//****************************************************************************

#include <iostream>
#include <string>

#include <sc/sc.hpp>              // For forward decls
#include <sc/source_location.hpp> // For SourceLocation struct

//****************************************************************************
namespace sc {
//****************************************************************************

SourceLocation::SourceLocation(unsigned int line, unsigned int column,
                               const std::string &file_name,
                               const std::string &function_name)
    : m_line(line), m_column(column), m_file_name(file_name),
      m_function_name(function_name) {}

SourceLocation::SourceLocation(const std::string &file_name)
    : SourceLocation(1, 0, file_name) {}

std::ostream &SourceLocation::dump(std::ostream &out) const {
  out << "SourceLocation: ";
  out << m_file_name;
  if (m_function_name != "") {
    out << "::" << m_function_name;
  }
  out << "::" << m_line;
  out << ":" << m_column;
  return out;
}

bool SourceLocation::operator==(SourceLocation const &other) const {
  return (m_line == other.m_line) && (m_column == other.m_column) &&
         (m_file_name == other.m_file_name) &&
         (m_function_name == other.m_function_name);
}

//****************************************************************************
} // namespace sc
//****************************************************************************
