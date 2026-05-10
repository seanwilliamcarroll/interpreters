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

#include <codegen/types.hpp>
#include <codegen/value.hpp>

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

struct LoopInformation {
  BlockLabel m_break_label;
  BlockLabel m_continue_label;
  Value m_break_returned_value;
};

struct BlockLabelStack {

  void push_scope(LoopInformation loop_info) {
    m_loop_information_stack.emplace_back(std::move(loop_info));
  }

  void pop_scope() noexcept {
    assert(!m_loop_information_stack.empty() &&
           "Cannot pop scope, already at global scope!");
    m_loop_information_stack.pop_back();
  }

  [[nodiscard]]
  const LoopInformation &current_loop_information() const {
    return m_loop_information_stack.back();
  }

private:
  std::vector<LoopInformation> m_loop_information_stack;
};

struct BlockLabelStackGuard {
  explicit BlockLabelStackGuard(BlockLabelStack &stack,
                                LoopInformation loop_info)
      : m_stack(stack) {
    m_stack.push_scope(std::move(loop_info));
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
