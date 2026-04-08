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

#include "codegen/symbol_table.hpp"
#include "operators.hpp"
#include <algorithm>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

// How to encode types, just strings?

enum class LLVMBinaryOperator : uint8_t {
  ADD,
  SUB,
  MUL,
  SDIV,
  SREM,
};

struct BinaryInstruction {
  Handle m_result;
  Handle m_lhs;
  Handle m_rhs;
  LLVMBinaryOperator m_operator;
  std::string m_type;
};

enum class BranchOperator : uint8_t {
  BNE, // ??
};

struct BranchInstruction {
  Handle m_label_a;
  Handle m_label_b;
  Handle m_condition;
  BranchOperator m_operator;
};

struct JumpInstruction {
  Handle m_target_label;
};

struct LoadInstruction {
  Handle m_destination;
  Handle m_source;
  std::string m_type;
};

struct StoreInstruction {
  Handle m_destination;
  Handle m_source;
  std::string m_type;
};

struct AllocaInstruction {
  Handle m_handle;
  std::string m_type;
};

struct ReturnInstruction {
  std::string m_type;
  Handle m_value;
};

using Instruction =
    std::variant<BinaryInstruction, // BranchInstruction, JumpInstruction,
                 LoadInstruction, StoreInstruction, AllocaInstruction,
                 ReturnInstruction>;

struct BasicBlock {

  void add_instruction(Instruction instruction) {
    m_instructions.push_back(std::move(instruction));
  }

  void add_terminal(Instruction instruction) {
    // Could assert on the terminals here
    m_terminal_instruction = std::move(instruction);
  }

  std::string m_label;
  std::vector<Instruction> m_instructions;
  Instruction m_terminal_instruction;
};

struct Function {

  BasicBlock &new_basic_block() {
    m_basic_blocks.emplace_back();
    return current_basic_block();
  }

  BasicBlock &current_basic_block() { return m_basic_blocks.back(); }

  void add_instruction(Instruction instruction) {
    current_basic_block().add_instruction(std::move(instruction));
  }

  void add_terminal(Instruction instruction) {
    current_basic_block().add_terminal(std::move(instruction));
    new_basic_block();
  }

  std::string m_function_id;
  std::string m_return_type;
  // TODO: Params
  std::vector<BasicBlock> m_basic_blocks;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
