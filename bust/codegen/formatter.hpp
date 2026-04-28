//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Formatter that serializes the in-memory IR to LLVM IR text.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/context.hpp>
#include <codegen/function.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/instructions.hpp>
#include <codegen/module.hpp>
#include <codegen/parameter.hpp>
#include <codegen/value.hpp>

#include <cstddef>
#include <iosfwd>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct HandleToString {

  std::string str(const Handle &);

  std::string operator()(const LiteralHandle &);
  std::string operator()(const TemporaryHandle &);
  std::string operator()(const NamedHandle &);
  std::string operator()(const GlobalHandle &);

  size_t m_temporary_count = 0;
  std::unordered_map<size_t, size_t> m_ssa_mapping;
};

struct Formatter {
  Formatter(const Context &ctx, std::ostream &out) : m_ctx(ctx), m_out(out) {}

  constexpr static const char *INDENT = "  ";

  void format(const auto &);

  std::string str(const LLVMType &);
  std::string str(TypeId);
  std::string str(const Index &);
  std::string str(const Handle &);
  std::string str(const Value &);

  void define_struct_type(const StructType &);
  void operator()(const Module &);

  void operator()(const Parameter &);
  void declare(const FunctionDeclaration &);
  void define(const FunctionDeclaration &);
  void operator()(const Function &);

  void operator()(const BasicBlock &);

  void operator()(const BinaryInstruction &);
  void operator()(const UnaryInstruction &);
  void operator()(const IntegerCompareInstruction &);
  void operator()(const LoadInstruction &);
  void operator()(const StoreInstruction &);
  void operator()(const CastInstruction &);
  void operator()(const GetElementPtrInstruction &);
  void operator()(const ExtractValueInstruction &);
  void operator()(const PtrToIntInstruction &);

  void operator()(const Argument &);
  void operator()(const CallVoidInstruction &);
  void operator()(const CallInstruction &);
  void operator()(const AllocaInstruction &);

  void operator()(const BranchInstruction &);
  void operator()(const JumpInstruction &);
  void operator()(const ReturnInstruction &);
  void operator()(const ReturnVoidInstruction &);

private:
  void newline() { m_out << "\n"; }

  void indent() { m_out << INDENT; }

  void newline_indent() {
    newline();
    indent();
  }

  void format_as_comma_separated_list(const auto &list_to_format,
                                      auto formatting_function) {
    if (list_to_format.empty()) {
      return;
    }
    for (size_t index = 0; index < list_to_format.size() - 1; ++index) {
      formatting_function(list_to_format[index]);
      m_out << ", ";
    }
    formatting_function(list_to_format.back());
  }

  const Context &m_ctx;
  std::ostream &m_out;
  HandleToString m_handle_converter{};
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
