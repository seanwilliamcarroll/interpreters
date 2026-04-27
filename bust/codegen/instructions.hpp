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
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <operators.hpp>

#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct AllocaInstruction {
  Value m_value_ptr;
  TypeId m_type_id;
};

struct StoreInstruction {
  Value m_destination;
  Value m_source;
};

struct LoadInstruction {
  Value m_destination;
  Value m_source;
};

struct GetElementPtrInstruction {
  Value m_destination;
  TypeId m_aggregate_type_id;
  Value m_ptr;
  Index m_initial_index;
  std::vector<Index> m_additional_indices;
};

struct PtrToIntInstruction {
  Value m_destination;
  Value m_source;
};

struct CallInstruction {
  Value m_destination;
  Value m_callee;
  std::vector<Value> m_arguments;
};

struct CallVoidInstruction {
  Value m_callee;
  std::vector<Value> m_arguments;
};

struct BranchInstruction {
  Value m_condition;
  BlockLabel m_iftrue;
  BlockLabel m_iffalse;
};

struct JumpInstruction {
  BlockLabel m_target;
};

struct IntegerCompareInstruction {
  Value m_destination;
  Value m_lhs;
  Value m_rhs;
  LLVMIntegerCompareCondition m_condition;
};

struct BinaryInstruction {
  Value m_destination;
  Value m_lhs;
  Value m_rhs;
  LLVMBinaryOperator m_operator;
};

struct UnaryInstruction {
  Value m_destination;
  Value m_source;
  UnaryOperator m_operator;
};

struct CastInstruction {
  Value m_destination;
  Value m_source;
  LLVMCastOperator m_operator;
};

struct ReturnInstruction {
  Value m_value;
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
