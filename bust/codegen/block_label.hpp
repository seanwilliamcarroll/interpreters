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

#include <string>

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

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
