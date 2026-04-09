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
#include "codegen/function_declaration.hpp"
#include "codegen/handle.hpp"
#include "codegen/instructions.hpp"
#include "codegen/symbol_table.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Function {
  Function(FunctionDeclaration signature) : m_signature(std::move(signature)) {
    set_insertion_point(new_basic_block("entry"));
  }

  FunctionDeclaration &signature() { return m_signature; }

  const FunctionDeclaration &signature() const { return m_signature; }

  const auto &basic_blocks() const { return m_basic_blocks; }

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

  void add_alloca_instruction(AllocaInstruction instruction) {
    entry_basic_block().add_alloca(m_alloca_insertion_position++,
                                   std::move(instruction));
  }

private:
  FunctionDeclaration m_signature;
  std::vector<std::unique_ptr<BasicBlock>> m_basic_blocks;
  BasicBlock *m_current_block = nullptr;
  size_t m_alloca_insertion_position = 0;
  UniqueNameTracker m_name_tracker;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
