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

struct Options {
  bool dump_ast = false;
  bool dump_hir = false;
  bool llvm_ir = false;
};

class Bust {
public:
  explicit Bust(std::istream &input, const char *filename,
                Options options = {});

  void run();

private:
  std::istream &m_input;
  const char *m_filename;
  Options m_options;
};

} // namespace bust

//****************************************************************************
