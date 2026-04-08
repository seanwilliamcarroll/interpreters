//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Instruction and terminator types for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/symbol_table.hpp"
#include "codegen/types.hpp"
#include <variant>

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

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
