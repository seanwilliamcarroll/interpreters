//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Instruction and terminator types for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/block_label.hpp>
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
  TypeId m_type;
};

struct UnaryInstruction {
  Handle m_result;
  Handle m_input;
  UnaryOperator m_operator;
  TypeId m_type;
};

struct IntegerCompareInstruction {
  Handle m_result;
  Handle m_lhs;
  Handle m_rhs;
  LLVMIntegerCompareCondition m_condition;
  TypeId m_type;
};

struct BranchInstruction {
  Handle m_condition;
  BlockLabel m_iftrue;
  BlockLabel m_iffalse;
};

struct JumpInstruction {
  BlockLabel m_target;
};

struct LoadInstruction {
  Handle m_destination;
  Handle m_source;
  TypeId m_type;
};

struct StoreInstruction {
  Handle m_destination;
  Handle m_source;
  TypeId m_type;
};

struct CastInstruction {
  Handle m_destination;
  Handle m_source;
  LLVMCastOperator m_operator;
  TypeId m_from;
  TypeId m_to;
};

struct GetElementPtrInstruction {
  Handle m_destination;
  TypeId m_struct_type;
  Handle m_struct_handle;
  Argument m_initial_index;
  std::vector<Argument> m_additional_indices;
};

struct PtrToIntInstruction {
  Handle m_destination;
  Handle m_source;
  TypeId m_destination_type;
};

struct CallVoidInstruction {
  Handle m_callee;
  std::vector<Argument> m_arguments;
};

struct CallInstruction {
  Handle m_target;
  Handle m_callee;
  std::vector<Argument> m_arguments;
  TypeId m_return_type;
};

struct AllocaInstruction {
  Handle m_handle;
  TypeId m_type;
};

struct ReturnInstruction {
  Handle m_value;
  TypeId m_type;
};

struct ReturnVoidInstruction {};

using Instruction =
    std::variant<BinaryInstruction, UnaryInstruction, IntegerCompareInstruction,
                 LoadInstruction, StoreInstruction, CastInstruction,
                 GetElementPtrInstruction, PtrToIntInstruction,
                 CallVoidInstruction, CallInstruction, AllocaInstruction>;

using Terminator = std::variant<BranchInstruction, JumpInstruction,
                                ReturnInstruction, ReturnVoidInstruction>;

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
