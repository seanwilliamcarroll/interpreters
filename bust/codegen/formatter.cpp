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
#include <utility>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void Formatter::operator()(const Function &function) {
  m_out << "define ";

  m_out << function.m_return_type;

  m_out << " @";

  m_out << function.m_function_id;

  // TODO
  m_out << "() {";

  newline();

  for (const auto &basic_block : function.m_basic_blocks) {
    (*this)(basic_block);
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

  std::visit(*this, basic_block.m_terminal_instruction);

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

  m_out << instruction.m_result;

  m_out << " = ";

  m_out << get_string(instruction.m_operator);

  m_out << " ";

  m_out << instruction.m_type;

  m_out << " ";

  m_out << instruction.m_lhs;

  m_out << ", ";

  m_out << instruction.m_rhs;

  newline();
}

void Formatter::operator()(const LoadInstruction &instruction) {
  indent();

  m_out << instruction.m_destination;

  m_out << " = load ";

  m_out << instruction.m_type;

  m_out << ", ptr ";

  m_out << instruction.m_source;

  newline();
}

void Formatter::operator()(const StoreInstruction &instruction) {
  indent();

  m_out << "store ";

  m_out << instruction.m_type;

  m_out << " ";

  m_out << instruction.m_source;

  m_out << ", ptr ";

  m_out << instruction.m_destination;

  newline();
}

void Formatter::operator()(const AllocaInstruction &instruction) {
  indent();

  m_out << instruction.m_handle;

  m_out << " = alloca ";

  m_out << instruction.m_type;

  newline();
}

void Formatter::operator()(const ReturnInstruction &instruction) {
  indent();

  m_out << "ret ";

  m_out << instruction.m_type;

  m_out << " ";

  m_out << instruction.m_value;

  newline();
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
