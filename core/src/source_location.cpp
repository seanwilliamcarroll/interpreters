//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
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

#include <sc/sc.hpp>              // For forward decls
#include <sc/source_location.hpp> // For SourceLocation struct

//****************************************************************************
namespace sc {
//****************************************************************************

SourceLocation::SourceLocation(const char *file_name, unsigned int line,
                               unsigned int column)
    : line(line), column(column), file_name(file_name) {}

std::ostream &operator<<(std::ostream &out, const SourceLocation &loc) {
  return out << "SourceLocation: " << loc.file_name << "::" << loc.line << ":"
             << loc.column;
}

bool operator==(const SourceLocation &loc_a, const SourceLocation &loc_b) {
  return loc_a.line == loc_b.line && loc_a.column == loc_b.column &&
         loc_a.file_name == loc_b.file_name;
}

//****************************************************************************
} // namespace sc
//****************************************************************************
