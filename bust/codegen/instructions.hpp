//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Instruction and terminator types for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/handle.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <operators.hpp>

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

struct UnaryInstruction {
  Handle m_result;
  Handle m_input;
  UnaryOperator m_operator;
  LLVMType m_type;
};

struct IntegerCompareInstruction {
  Handle m_result;
  Handle m_lhs;
  Handle m_rhs;
  LLVMIntegerCompareCondition m_condition;
  LLVMType m_type;
};

struct BranchInstruction {
  Handle m_condition;
  Handle m_iftrue;
  Handle m_iffalse;
};

struct JumpInstruction {
  Handle m_target;
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

struct CastInstruction {
  Handle m_destination;
  Handle m_source;
  LLVMCastOperator m_operator;
  LLVMType m_from;
  LLVMType m_to;
};

struct CallVoidInstruction {
  Handle m_callee;
  std::vector<Argument> m_arguments;
};

struct CallInstruction {
  Handle m_target;
  Handle m_callee;
  std::vector<Argument> m_arguments;
  LLVMType m_return_type;
};

struct AllocaInstruction {
  Handle m_handle;
  LLVMType m_type;
};

struct ReturnInstruction {
  Handle m_value;
  LLVMType m_type;
};

struct ReturnVoidInstruction {};

using Instruction =
    std::variant<BinaryInstruction, UnaryInstruction, IntegerCompareInstruction,
                 LoadInstruction, StoreInstruction, CastInstruction,
                 CallVoidInstruction, CallInstruction, AllocaInstruction>;

using Terminator = std::variant<BranchInstruction, JumpInstruction,
                                ReturnInstruction, ReturnVoidInstruction>;

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
