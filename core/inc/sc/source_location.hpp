//********* Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
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

#include <string>
#include <iostream>

//****************************************************************************
namespace sc {
//****************************************************************************


struct SourceLocation{
  // std::source_location is in C++20, which I don't think we currently support?

  SourceLocation(unsigned int line, unsigned int column, const std::string& file_name,
                 const std::string& function_name = "") :
    m_line(line), m_column(column),
    m_file_name(file_name), m_function_name(function_name) {}

  SourceLocation(const std::string& file_name) :
    SourceLocation(1, 0, file_name) {}
  
  std::ostream& dump(std::ostream& out) const {
    out << "SourceLocation: ";
    out << m_file_name;
    if (m_function_name != "") {
      out << "::" << m_function_name;
    }
    out << "::" << m_line;
    out << ":" << m_column;
    return out;
  }

  unsigned int m_line;
  unsigned int m_column;
  std::string m_file_name;
  std::string m_function_name;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
