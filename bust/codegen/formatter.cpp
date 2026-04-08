//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Formatter implementation.
//*
//*
//****************************************************************************

#include "codegen/formatter.hpp"
#include "codegen/basic_block.hpp"
#include "codegen/types.hpp"
#include "exceptions.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void Formatter::operator()(const Module &mod) {
  // TODO: Globals
  for (const auto &function : mod.m_functions) {
    (*this)(*function);
  }
}

void Formatter::operator()(const Function &function) {
  m_out << "define " << function.m_return_type << " @"
        << function.m_function_id;

  // TODO
  m_out << "() {";

  newline();

  for (const auto &basic_block : function.m_basic_blocks) {
    (*this)(*basic_block);
  }

  m_out << "}";
  newline();
  newline();
}

void Formatter::operator()(const BasicBlock &basic_block) {
  // TODO: Label

  for (const auto &instruction : basic_block.m_instructions) {
    std::visit(*this, instruction);
  }

  if (!basic_block.m_terminal_instruction.has_value()) {
    throw std::runtime_error("Found basic block without terminal instruction!");
  }

  std::visit(*this, basic_block.m_terminal_instruction.value());

  newline();
}

const char *get_string(LLVMBinaryOperator op) {
  switch (op) {
  case LLVMBinaryOperator::ADD:
    return "add";
  case LLVMBinaryOperator::SUB:
    return "sub";
  case LLVMBinaryOperator::MUL:
    return "mul";
  case LLVMBinaryOperator::SDIV:
    return "sdiv";
  case LLVMBinaryOperator::SREM:
    return "srem";
  }

  std::unreachable();
}

void Formatter::operator()(const BinaryInstruction &instruction) {
  indent();

  m_out << instruction.m_result << " = " << get_string(instruction.m_operator)
        << " " << instruction.m_type << " " << instruction.m_lhs << ", "
        << instruction.m_rhs;

  newline();
}

void Formatter::operator()(const LoadInstruction &instruction) {
  indent();

  m_out << instruction.m_destination << " = load " << instruction.m_type
        << ", ptr " << instruction.m_source;

  newline();
}

void Formatter::operator()(const StoreInstruction &instruction) {
  indent();

  m_out << "store " << instruction.m_type << " " << instruction.m_source
        << ", ptr " << instruction.m_destination;

  newline();
}

void Formatter::operator()(const AllocaInstruction &instruction) {
  indent();

  m_out << instruction.m_handle << " = alloca " << instruction.m_type;

  newline();
}

void Formatter::operator()(const BranchInstruction &) {}

void Formatter::operator()(const JumpInstruction &) {}

void Formatter::operator()(const ReturnInstruction &instruction) {
  indent();

  m_out << "ret " << instruction.m_type;
  if (instruction.m_type != LLVMType::VOID) {
    m_out << " " << instruction.m_value;
  }

  newline();
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
