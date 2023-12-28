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

#include <iostream>
#include <string>

//****************************************************************************
namespace sc {
//****************************************************************************

struct SourceLocation {
  // std::source_location is in C++20, which I don't think we currently support?

  SourceLocation(unsigned int line, unsigned int column,
                 const std::string &file_name,
                 const std::string &function_name = "");

  SourceLocation(const std::string &file_name);

  SourceLocation();

  std::ostream &dump(std::ostream &out) const;
  bool operator==(SourceLocation const &other) const;

  unsigned int m_line;
  unsigned int m_column;
  std::string m_file_name;
  std::string m_function_name;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
