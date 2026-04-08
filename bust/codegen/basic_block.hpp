//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Basic block representation for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/instructions.hpp"
#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct BasicBlock {
  void add_instruction(Instruction instruction) {
    m_instructions.push_back(std::move(instruction));
  }

  void add_terminal(Terminator terminator) {
    assert(!m_terminal_instruction.has_value() && "Shouldn't set this twice!");
    m_terminal_instruction = std::move(terminator);
  }

  std::string m_label{};
  std::vector<Instruction> m_instructions{};
  std::optional<Terminator> m_terminal_instruction{};
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
