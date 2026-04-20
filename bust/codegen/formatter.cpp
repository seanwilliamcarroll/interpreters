//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Formatter implementation.
//*
//*
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/formatter.hpp>
#include <codegen/function.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/handle.hpp>
#include <codegen/instructions.hpp>
#include <codegen/module.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <operators.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

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

std::string HandleToString::operator()(const LocalHandle &handle) {
  return "%" + handle.m_handle;
}

std::string HandleToString::operator()(const ParameterHandle &handle) {
  return "%" + handle.m_handle;
}

std::string HandleToString::operator()(const GlobalHandle &handle) {
  return "@" + handle.m_handle;
}

std::string HandleToString::operator()(const TypeHandle &handle) {
  return "%" + handle.m_handle;
}

void Formatter::format(const auto &to_format) { (*this)(to_format); }

void Formatter::operator()(const Module &mod) {
  // TODO: Globals

  m_out << ";------------------------------------------------------------------"
           "--------------\n";

  for (const auto &[struct_type_id, _] : m_ctx.type().struct_names()) {
    define_struct_type(struct_type_id);
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

void Formatter::define_struct_type(TypeId struct_type_id) {

  m_out << m_ctx.to_string(struct_type_id) << " = type { ";

  const auto &struct_type =
      m_ctx.type().as<StructType>(struct_type_id, __PRETTY_FUNCTION__);

  for (size_t index = 0; index < struct_type.m_fields.size() - 1; ++index) {
    m_out << m_ctx.to_string(struct_type.m_fields[index]) << ", ";
  }
  m_out << m_ctx.to_string(struct_type.m_fields.back());

  m_out << " }";

  newline();
  newline();
}

void Formatter::operator()(const Parameter &parameter) {
  m_out << m_ctx.to_string(parameter.m_type) << " "
        << m_handle_converter(parameter.m_name);
}

void Formatter::function_parameters(const FunctionDeclaration &signature) {
  if (signature.m_parameters.empty()) {
    return;
  }
  for (size_t index = 0; index < signature.m_parameters.size() - 1; ++index) {
    const auto &parameter = signature.m_parameters[index];
    format(parameter);
    m_out << ", ";
  }
  format(signature.m_parameters.back());
}

void Formatter::declare(const FunctionDeclaration &signature) {
  m_out << "declare " << m_ctx.to_string(signature.m_return_type) << " "
        << std::visit(m_handle_converter, signature.m_function_id);

  m_out << "(";

  function_parameters(signature);

  m_out << ")";

  newline();
  newline();
}

void Formatter::define(const FunctionDeclaration &signature) {
  m_out << "define " << m_ctx.to_string(signature.m_return_type) << " "
        << std::visit(m_handle_converter, signature.m_function_id);

  m_out << "(";

  function_parameters(signature);

  m_out << ")";
}

void Formatter::operator()(const Function &function) {
  // Reset for each function for readability
  m_handle_converter = HandleToString{};

  m_out << std::visit(m_handle_converter, function.signature().m_function_id) +
               ".closure = constant " + m_ctx.to_string(m_ctx.m_fat_ptr) +
               " { ptr "
        << std::visit(m_handle_converter, function.signature().m_function_id)
        << ", ptr null }";
  newline();

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

  m_out << get_raw_handle(basic_block.label()) << ":\n";

  for (const auto &instruction : basic_block.instructions()) {
    std::visit(*this, instruction);
  }

  if (!basic_block.terminal().has_value()) {
    throw std::runtime_error("Found basic block without terminal instruction!");
  }

  std::visit(*this, basic_block.terminal().value());

  newline();
}

void Formatter::operator()(const BinaryInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_result) << " = "
        << instruction.m_operator << " " << m_ctx.to_string(instruction.m_type)
        << " " << std::visit(m_handle_converter, instruction.m_lhs) << ", "
        << std::visit(m_handle_converter, instruction.m_rhs);

  newline();
}

void Formatter::operator()(const UnaryInstruction &instruction) {
  indent();

  switch (instruction.m_operator) {
  case UnaryOperator::MINUS:
    m_out << std::visit(m_handle_converter, instruction.m_result) << " = sub "
          << " " << m_ctx.to_string(instruction.m_type) << " 0, "
          << std::visit(m_handle_converter, instruction.m_input);
    break;
  case UnaryOperator::NOT:
    const char *true_equivalent =
        (m_ctx.m_i1 == instruction.m_type) ? "true" : "-1";
    m_out << std::visit(m_handle_converter, instruction.m_result) << " = xor "
          << " " << m_ctx.to_string(instruction.m_type) << " "
          << true_equivalent << ", "
          << std::visit(m_handle_converter, instruction.m_input);
    break;
  }

  newline();
}

void Formatter::operator()(const IntegerCompareInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_result) << " = icmp "
        << instruction.m_condition << " " << m_ctx.to_string(instruction.m_type)
        << " " << std::visit(m_handle_converter, instruction.m_lhs) << ", "
        << std::visit(m_handle_converter, instruction.m_rhs);

  newline();
}

void Formatter::operator()(const LoadInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_destination)
        << " = load " << m_ctx.to_string(instruction.m_type) << ", ptr "
        << std::visit(m_handle_converter, instruction.m_source);

  newline();
}

void Formatter::operator()(const StoreInstruction &instruction) {
  indent();

  m_out << "store " << m_ctx.to_string(instruction.m_type) << " "
        << std::visit(m_handle_converter, instruction.m_source) << ", ptr "
        << std::visit(m_handle_converter, instruction.m_destination);

  newline();
}

void Formatter::operator()(const CastInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_destination) << " = "
        << instruction.m_operator << " " << m_ctx.to_string(instruction.m_from)
        << " " << std::visit(m_handle_converter, instruction.m_source) << " to "
        << m_ctx.to_string(instruction.m_to);

  newline();
}

void Formatter::operator()(const GetElementPtrInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_destination)
        << " = getelementptr " << m_ctx.to_string(instruction.m_struct_type)
        << ", ptr "
        << std::visit(m_handle_converter, instruction.m_struct_handle) << ", "
        << m_ctx.to_string(instruction.m_initial_index.m_type) << " "
        << std::visit(m_handle_converter, instruction.m_initial_index.m_name);

  for (const auto &additional_index : instruction.m_additional_indices) {
    m_out << ", " << m_ctx.to_string(additional_index.m_type) << " "
          << std::visit(m_handle_converter, additional_index.m_name);
  }

  newline();
}

void Formatter::operator()(const PtrToIntInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_destination)
        << " = ptrtoint ptr "
        << std::visit(m_handle_converter, instruction.m_source) << " to "
        << m_ctx.to_string(instruction.m_destination_type);

  newline();
}

void Formatter::operator()(const Argument &argument) {
  m_out << m_ctx.to_string(argument.m_type) << " "
        << std::visit(m_handle_converter, argument.m_name);
}

void Formatter::function_arguments(const std::vector<Argument> &arguments) {
  if (arguments.empty()) {
    return;
  }
  for (size_t index = 0; index < arguments.size() - 1; ++index) {
    const auto &argument = arguments[index];
    format(argument);
    m_out << ", ";
  }
  format(arguments.back());
}

void Formatter::operator()(const CallVoidInstruction &instruction) {
  indent();

  m_out << "call void " << std::visit(m_handle_converter, instruction.m_callee);

  m_out << "(";
  function_arguments(instruction.m_arguments);
  m_out << ")";

  newline();
}

void Formatter::operator()(const CallInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_target) << " = call "
        << m_ctx.to_string(instruction.m_return_type) << " "
        << std::visit(m_handle_converter, instruction.m_callee);

  m_out << "(";
  function_arguments(instruction.m_arguments);
  m_out << ")";

  newline();
}

void Formatter::operator()(const AllocaInstruction &instruction) {
  indent();

  m_out << std::visit(m_handle_converter, instruction.m_handle) << " = alloca "
        << m_ctx.to_string(instruction.m_type);

  newline();
}

void Formatter::operator()(const BranchInstruction &instruction) {
  indent();

  m_out << "br i1 " << std::visit(m_handle_converter, instruction.m_condition)
        << ", label " << std::visit(m_handle_converter, instruction.m_iftrue)
        << ", label " << std::visit(m_handle_converter, instruction.m_iffalse);

  newline();
}

void Formatter::operator()(const JumpInstruction &instruction) {
  indent();

  m_out << "br label " << std::visit(m_handle_converter, instruction.m_target);

  newline();
}

void Formatter::operator()(const ReturnVoidInstruction & /*unused*/) {
  indent();
  m_out << "ret void";
  newline();
}

void Formatter::operator()(const ReturnInstruction &instruction) {
  indent();

  m_out << "ret " << m_ctx.to_string(instruction.m_type) << " "
        << std::visit(m_handle_converter, instruction.m_value);

  newline();
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
