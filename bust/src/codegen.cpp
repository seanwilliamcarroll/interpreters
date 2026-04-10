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
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

std::string CodeGen::operator()(const hir::Program &program) {
  auto context = bust::codegen::Context{};

  auto collector = bust::codegen::TopItemDeclarationCollector{context};
  auto generator = bust::codegen::TopItemGenerator{context};

  for (const auto &top_item : program.m_top_items) {
    std::visit(collector, (top_item));
  }

  for (const auto &top_item : program.m_top_items) {
    std::visit(generator, (top_item));
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
