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
#include "codegen/types.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Function {
  Function() { set_insertion_point(new_basic_block()); }

  BasicBlock &new_basic_block() {
    m_basic_blocks.emplace_back(std::make_unique<BasicBlock>());
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

  std::string m_function_id;
  LLVMType m_return_type;
  // TODO: Params
  std::vector<std::unique_ptr<BasicBlock>> m_basic_blocks;
  BasicBlock *m_current_block = nullptr;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
