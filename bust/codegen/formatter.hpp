//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Formatter that serializes the in-memory IR to LLVM IR text.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/basic_block.hpp"
#include <iosfwd>
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Formatter {
  Formatter(std::ostream &out) : m_out(out) {}

  constexpr static const char *INDENT = "  ";

  void operator()(const Function &);
  void operator()(const BasicBlock &);
  void operator()(const BinaryInstruction &);
  void operator()(const LoadInstruction &);
  void operator()(const StoreInstruction &);
  void operator()(const AllocaInstruction &);
  void operator()(const ReturnInstruction &);

  void newline() { m_out << "\n"; }

  void indent() { m_out << INDENT; }

  void newline_indent() {
    newline();
    indent();
  }

private:
  std::ostream &m_out;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
