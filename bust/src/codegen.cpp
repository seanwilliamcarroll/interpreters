//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : LLVM IR code generation pass implementation.
//*
//*
//****************************************************************************

#include <codegen.hpp>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <codegen/context.hpp>
#include <codegen/formatter.hpp>
#include <codegen/top_item_generator.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

std::string CodeGen::operator()(const zir::Program &program) {
  auto context = codegen::Context(program.m_arena);

  auto collector = codegen::TopItemDeclarationCollector{context};
  auto generator = codegen::TopItemGenerator{context};

  for (const auto &top_item : program.m_top_items) {
    collector.collect(top_item);
  }

  for (const auto &top_item : program.m_top_items) {
    generator.generate(top_item);
  }

  // How to do top level let bindings?
  std::stringstream out;
  auto formatter = codegen::Formatter(out);
  formatter(context.module());

  return out.str();
}

//****************************************************************************
} // namespace bust
//****************************************************************************
