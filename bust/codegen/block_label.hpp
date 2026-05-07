//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Typed label referring to a BasicBlock. Constructed only by
//*            IRBuilder; safe to store in instructions as a jump target.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <cassert>
#include <string>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct BasicBlock;

struct BlockLabel {
  [[nodiscard]] const std::string &name() const;

  static BlockLabel null();

private:
  explicit BlockLabel(BasicBlock *block) : m_block(block) {}

  BasicBlock *m_block;

  friend struct IRBuilder;
};

struct BlockLabelStack {

  void push_scope(BlockLabel label) { m_block_labels.push_back(label); }
  void pop_scope() noexcept {
    assert(!m_block_labels.empty() &&
           "Cannot pop scope, already at global scope!");
    m_block_labels.pop_back();
  }

  [[nodiscard]]
  const BlockLabel &current_block_label() const {
    return m_block_labels.back();
  }

private:
  std::vector<BlockLabel> m_block_labels;
};

struct BlockLabelStackGuard {
  explicit BlockLabelStackGuard(BlockLabelStack &stack, BlockLabel label)
      : m_stack(stack) {
    m_stack.push_scope(label);
  }
  ~BlockLabelStackGuard() { m_stack.pop_scope(); }

  BlockLabelStackGuard(const BlockLabelStackGuard &) = delete;
  BlockLabelStackGuard &operator=(const BlockLabelStackGuard &) = delete;
  BlockLabelStackGuard(BlockLabelStackGuard &&) = delete;
  BlockLabelStackGuard &operator=(BlockLabelStackGuard &&) = delete;

private:
  BlockLabelStack &m_stack;
};
//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
