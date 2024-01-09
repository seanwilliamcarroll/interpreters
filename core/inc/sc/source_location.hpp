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

#include <iosfwd>
#include <string>

//****************************************************************************
namespace sc {
//****************************************************************************

struct SourceLocation {

  SourceLocation(const char *file_name = "", unsigned int line = 1,
                 unsigned int column = 0);

  const unsigned int line;
  const unsigned int column;
  const std::string file_name;
};

std::ostream &operator<<(std::ostream &, const SourceLocation &);
bool operator==(const SourceLocation &, const SourceLocation &);

//****************************************************************************
} // namespace sc
//****************************************************************************
