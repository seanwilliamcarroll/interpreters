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
#include "codegen/types.hpp"
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct BinaryInstruction {
  Handle m_result;
  Handle m_lhs;
  Handle m_rhs;
  LLVMBinaryOperator m_operator;
  LLVMType m_type;
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
  LLVMType m_type;
};

struct StoreInstruction {
  Handle m_destination;
  Handle m_source;
  LLVMType m_type;
};

struct AllocaInstruction {
  Handle m_handle;
  LLVMType m_type;
};

struct ReturnInstruction {
  Handle m_value;
  LLVMType m_type;
};

using Instruction = std::variant<BinaryInstruction, LoadInstruction,
                                 StoreInstruction, AllocaInstruction>;

using Terminator =
    std::variant<BranchInstruction, JumpInstruction, ReturnInstruction>;

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

struct Global {
  // TODO
};

struct Function {
  BasicBlock &new_basic_block() {
    m_basic_blocks.emplace_back(std::make_unique<BasicBlock>());
    return *m_basic_blocks.back();
  }

  void set_insertion_point(BasicBlock &basic_block) {
    m_current_block = &basic_block;
  }

  BasicBlock &current_basic_block() { return *m_current_block; }

  void add_instruction(Instruction instruction) {
    current_basic_block().add_instruction(std::move(instruction));
  }

  void add_terminal(Terminator terminator) {
    current_basic_block().add_terminal(std::move(terminator));
  }

  std::string m_function_id;
  std::string m_return_type;
  // TODO: Params
  std::vector<std::unique_ptr<BasicBlock>> m_basic_blocks;
  BasicBlock *m_current_block = nullptr;
};

struct Module {
  Function &new_function() {
    m_functions.emplace_back(std::make_unique<Function>());
    return *m_functions.back();
  }

  Function &current_function() { return *m_functions.back(); }

  BasicBlock &current_basic_block() {
    return current_function().current_basic_block();
  }

  std::vector<Global> m_globals;
  std::vector<std::unique_ptr<Function>> m_functions;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
