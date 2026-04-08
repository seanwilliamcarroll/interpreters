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

  auto generator = bust::codegen::TopItemGenerator{context};

  for (const auto &top_item : program.m_top_items) {
    std::visit(generator, (top_item));
  }

  // How to do top level let bindings?
  std::stringstream out;
  auto formatter = codegen::Formatter(out);
  for (const auto &top_level_function : context.m_top_level_functions) {
    formatter(top_level_function);
  }

  return out.str();
}

//****************************************************************************
} // namespace bust
//****************************************************************************
