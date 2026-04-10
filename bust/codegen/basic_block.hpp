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

#include <cassert>
#include <codegen/instructions.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct BasicBlock {
  BasicBlock(const Handle &label) : m_label(label) {}

  const Handle &label() const { return m_label; }

  const auto &instructions() const { return m_instructions; }

  const auto &terminal() const { return m_terminal_instruction; }

  void add_instruction(Instruction instruction) {
    m_instructions.push_back(std::move(instruction));
  }

  void add_terminal(Terminator terminator) {
    assert(!m_terminal_instruction.has_value() && "Shouldn't set this twice!");
    m_terminal_instruction = std::move(terminator);
  }

  void add_alloca(size_t position, AllocaInstruction instruction) {
    m_instructions.insert(
        m_instructions.begin() +
            static_cast<decltype(m_instructions)::difference_type>(position),
        std::move(instruction));
  }

private:
  Handle m_label;
  std::vector<Instruction> m_instructions{};
  std::optional<Terminator> m_terminal_instruction{};
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
