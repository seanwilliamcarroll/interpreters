//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Function representation for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/basic_block.hpp"
#include "codegen/instructions.hpp"
#include "codegen/symbol_table.hpp"
#include "codegen/types.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Function {
  Function(const Handle &function_id, LLVMType return_type)
      : m_function_id(function_id), m_return_type(return_type) {
    set_insertion_point(new_basic_block("entry"));
  }

  BasicBlock &new_basic_block(const std::string &label) {
    m_basic_blocks.emplace_back(std::make_unique<BasicBlock>(
        LocalHandle{m_name_tracker.uniquify(label)}));
    return *m_basic_blocks.back();
  }

  void set_insertion_point(BasicBlock &basic_block) {
    m_current_block = &basic_block;
  }

  // First block is always the entry block for alloca purposes
  BasicBlock &entry_basic_block() { return *m_basic_blocks.front(); }

  BasicBlock &current_basic_block() { return *m_current_block; }

  void add_instruction(Instruction instruction) {
    current_basic_block().add_instruction(std::move(instruction));
  }

  void add_terminal(Terminator terminator) {
    current_basic_block().add_terminal(std::move(terminator));
  }

  void add_alloca_instruction(AllocaInstruction instruction) {
    entry_basic_block().add_alloca(m_alloca_insertion_position++,
                                   std::move(instruction));
  }

  Handle m_function_id;
  LLVMType m_return_type;
  // TODO: Params
  std::vector<std::unique_ptr<BasicBlock>> m_basic_blocks;
  BasicBlock *m_current_block = nullptr;
  size_t m_alloca_insertion_position = 0;
  UniqueNameTracker m_name_tracker;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
