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

#include <cstdint>
#include <iostream>

//****************************************************************************

namespace bust {

enum class Mode : std::uint8_t {
  RUN,
  DUMP_AST,
  DUMP_HIR,
  EVAL,
  LLVM_IR,
};

class Bust {
public:
  explicit Bust(std::istream &input, const char *filename = "<stdin>",
                Mode mode = Mode::RUN);

  void rep();

private:
  std::istream &m_input;
  const char *m_filename;
  Mode m_mode;
};

} // namespace bust

//****************************************************************************
