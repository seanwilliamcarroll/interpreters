//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : LLVM IR code generation pass implementation.
//*
//*
//****************************************************************************

#include "codegen/context.hpp"
#include "codegen/formatter.hpp"
#include "codegen/top_item_generator.hpp"
#include <codegen.hpp>
#include <concepts>
#include <sstream>

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
  formatter(context.m_module);

  return out.str();
}

//****************************************************************************
} // namespace bust
//****************************************************************************
