//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Top-level driver class for the bust language.
//*
//*
//****************************************************************************

#pragma once

#include <iostream>

//****************************************************************************

namespace bust {

class Bust {
public:
  explicit Bust(std::istream &input, const char *filename = "<stdin>");

  void rep();

private:
  std::istream &m_input;
  const char *m_filename;
};

} // namespace bust

//****************************************************************************
