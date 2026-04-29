//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Formatter implementation.
//*
//*
//****************************************************************************

#include <codegen/arena.hpp>
#include <codegen/basic_block.hpp>
#include <codegen/block_label.hpp>
#include <codegen/context.hpp>
#include <codegen/formatter.hpp>
#include <codegen/function.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/instructions.hpp>
#include <codegen/ir_literals.hpp>
#include <codegen/ir_syntax.hpp>
#include <codegen/module.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <operators.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

std::string format_block_ref(BlockLabel block_label) {
  return std::string{ir_syntax::percent_symbol} + block_label.name();
}

std::string HandleToString::str(const Handle &handle) {
  return std::visit(*this, handle);
}

std::string HandleToString::operator()(const LiteralHandle &handle) {
  return handle.m_handle;
}

std::string HandleToString::operator()(const TemporaryHandle &handle) {
  auto iter = m_ssa_mapping.find(handle.m_id);

  if (iter != m_ssa_mapping.end()) {
    return "%" + std::to_string(iter->second);
  }

  auto new_mapping = m_temporary_count++;
  m_ssa_mapping[handle.m_id] = new_mapping;

  return "%" + std::to_string(new_mapping);
}

std::string HandleToString::operator()(const NamedHandle &handle) {
  return "%" + handle.m_handle;
}

std::string HandleToString::operator()(const GlobalHandle &handle) {
  return "@" + handle.m_handle;
}

void Formatter::format(const auto &to_format) { (*this)(to_format); }

std::string Formatter::str(const LLVMType &type) {
  return m_ctx.to_string(type);
}

std::string Formatter::str(TypeId type_id) { return m_ctx.to_string(type_id); }

std::string Formatter::str(const Index &index) {
  return str(index.m_type) + " " + std::to_string(index.m_index);
}

std::string Formatter::str(const Handle &handle) {
  return m_handle_converter.str(handle);
}

std::string Formatter::str(const Value &value) {
  return str(value.m_type_id) + " " + str(value.m_handle);
}

void Formatter::operator()(const Module &mod) {
  // TODO: Globals

  m_out << ";------------------------------------------------------------------"
           "--------------\n";

  for (const auto &struct_type : m_ctx.type().named_struct_types()) {
    define_struct_type(struct_type);
  }

  m_out << ";------------------------------------------------------------------"
           "--------------\n";

  for (const auto &function_declaration : mod.extern_functions()) {
    declare(*function_declaration);
  }

  m_out << ";------------------------------------------------------------------"
           "--------------\n";

  for (const auto &function : mod.functions()) {
    format(*function);
  }
}

void Formatter::define_struct_type(const StructType &struct_type) {

  m_out << str(struct_type) << " = " << ir_syntax::type_keyword << " { ";

  format_as_comma_separated_list(
      struct_type.m_fields, [&](const auto &field) { m_out << str(field); });

  m_out << " }";

  newline();
  newline();
}

void Formatter::operator()(const Parameter &parameter) {
  m_out << str(parameter.m_type) << " %" << parameter.m_name;
}

void Formatter::declare(const FunctionDeclaration &signature) {
  m_out << ir_syntax::declare << " " << str(signature.m_return_type) << " "
        << str(signature.m_function_id.m_handle);

  m_out << "(";

  format_as_comma_separated_list(
      signature.m_parameters,
      [&](const auto &parameter) { format(parameter); });

  m_out << ")";

  newline();
  newline();
}

void Formatter::define(const FunctionDeclaration &signature) {
  m_out << ir_syntax::define << " " << str(signature.m_return_type) << " "
        << str(signature.m_function_id.m_handle);

  m_out << "(";

  format_as_comma_separated_list(
      signature.m_parameters,
      [&](const auto &parameter) { format(parameter); });

  m_out << ")";
}

void Formatter::operator()(const Function &function) {
  // Reset for each function for readability
  m_handle_converter = HandleToString{};

  define(function.signature());

  m_out << " {";

  newline();

  for (const auto &basic_block : function.basic_blocks()) {
    format(*basic_block);
  }

  m_out << "}";
  newline();
  newline();
}

void Formatter::operator()(const BasicBlock &basic_block) {

  m_out << basic_block.label() << ":\n";

  for (const auto &instruction : basic_block.instructions()) {
    std::visit(*this, instruction);
  }

  if (!basic_block.terminal().has_value()) {
    throw std::runtime_error("Found basic block without terminal instruction!");
  }

  std::visit(*this, basic_block.terminal().value());

  newline();
}

void Formatter::operator()(const AllocaInstruction &instruction) {
  indent();

  m_out << str(instruction.m_value_ptr.m_handle) << " = "
        << ir_syntax::alloca_op << " " << str(instruction.m_type_id);

  newline();
}

void Formatter::operator()(const StoreInstruction &instruction) {
  indent();

  m_out << ir_syntax::store << " " << str(instruction.m_source) << ", "
        << str(instruction.m_destination);

  newline();
}

void Formatter::operator()(const LoadInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = " << ir_syntax::load
        << " " << str(instruction.m_destination.m_type_id) << ", "
        << str(instruction.m_source);

  newline();
}

void Formatter::operator()(const GetElementPtrInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = "
        << ir_syntax::getelementptr << " "
        << str(instruction.m_aggregate_type_id) << ", "
        << str(instruction.m_ptr) << ", " << str(instruction.m_initial_index);

  for (const auto &additional_index : instruction.m_additional_indices) {
    m_out << ", " << str(additional_index);
  }

  newline();
}

void Formatter::operator()(const ExtractValueInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = "
        << ir_syntax::extractvalue << " "
        << str(instruction.m_aggregate_type_id) << " "
        << str(instruction.m_source.m_handle) << ", "
        << std::to_string(instruction.m_index);

  newline();
}

void Formatter::operator()(const PtrToIntInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = "
        << ir_syntax::ptrtoint << " " << str(instruction.m_source) << " "
        << ir_syntax::to << " " << str(instruction.m_destination.m_type_id);

  newline();
}

void Formatter::operator()(const CallInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = " << ir_syntax::call
        << " " << str(instruction.m_destination.m_type_id) << " "
        << str(instruction.m_callee.m_handle);

  m_out << "(";
  format_as_comma_separated_list(
      instruction.m_arguments,
      [&](const auto &argument) { m_out << str(argument); });
  m_out << ")";

  newline();
}

void Formatter::operator()(const CallVoidInstruction &instruction) {
  indent();

  m_out << ir_syntax::call_void << " " << str(instruction.m_callee.m_handle);

  m_out << "(";

  format_as_comma_separated_list(
      instruction.m_arguments,
      [&](const auto &argument) { m_out << str(argument); });

  m_out << ")";

  newline();
}

void Formatter::operator()(const BranchInstruction &instruction) {
  indent();

  m_out << ir_syntax::br << " " << str(instruction.m_condition) << ", "
        << ir_syntax::label << " " << format_block_ref(instruction.m_iftrue)
        << ", " << ir_syntax::label << " "
        << format_block_ref(instruction.m_iffalse);

  newline();
}

void Formatter::operator()(const JumpInstruction &instruction) {
  indent();

  m_out << ir_syntax::br << " " << ir_syntax::label << " "
        << format_block_ref(instruction.m_target);

  newline();
}

void Formatter::operator()(const IntegerCompareInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = " << ir_syntax::icmp
        << " " << instruction.m_condition << " "
        << str(instruction.m_lhs.m_type_id) << " "
        << str(instruction.m_lhs.m_handle) << ", "
        << str(instruction.m_rhs.m_handle);

  newline();
}

void Formatter::operator()(const BinaryInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = "
        << instruction.m_operator << " "
        << str(instruction.m_destination.m_type_id) << " "
        << str(instruction.m_lhs.m_handle) << ", "
        << str(instruction.m_rhs.m_handle);

  newline();
}

void Formatter::operator()(const UnaryInstruction &instruction) {
  indent();

  switch (instruction.m_operator) {
  case UnaryOperator::MINUS:
    m_out << str(instruction.m_destination.m_handle) << " = " << ir_syntax::sub
          << "  " << str(instruction.m_destination.m_type_id) << " "
          << ir_literals::zero << ", " << str(instruction.m_source.m_handle);
    break;
  case UnaryOperator::NOT:
    const auto &true_equivalent =
        (m_ctx.m_i1 == instruction.m_destination.m_type_id)
            ? ir_literals::true_
            : ir_literals::all_ones;
    m_out << str(instruction.m_destination.m_handle) << " = "
          << ir_syntax::xor_op << "  "
          << str(instruction.m_destination.m_type_id) << " " << true_equivalent
          << ", " << str(instruction.m_source.m_handle);
    break;
  }

  newline();
}

void Formatter::operator()(const CastInstruction &instruction) {
  indent();

  m_out << str(instruction.m_destination.m_handle) << " = "
        << instruction.m_operator << " " << str(instruction.m_source) << " "
        << ir_syntax::to << " " << str(instruction.m_destination.m_type_id);

  newline();
}

void Formatter::operator()(const ReturnInstruction &instruction) {
  indent();

  m_out << ir_syntax::ret << " " << str(instruction.m_value);

  newline();
}

void Formatter::operator()(const ReturnVoidInstruction & /*unused*/) {
  indent();
  m_out << ir_syntax::ret_void;
  newline();
}

void Formatter::operator()(const Argument &argument) {
  m_out << str(argument.m_type) << " " << str(argument.m_name);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
