//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Module representation for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/basic_block.hpp"
#include "codegen/function.hpp"
#include "codegen/instructions.hpp"
#include <memory>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Global {
  // TODO
};

struct Module {
  Function &new_function(FunctionDeclaration signature) {
    m_functions.emplace_back(std::make_unique<Function>(std::move(signature)));
    return *m_functions.back();
  }

  Function &current_function() { return *m_current_function; }

  void set_current_function(Function &function) {
    m_current_function = &function;
  }

  BasicBlock &current_basic_block() {
    return current_function().current_basic_block();
  }

  std::vector<Global> m_globals;
  std::vector<std::unique_ptr<FunctionDeclaration>> m_signatures;
  std::vector<std::unique_ptr<Function>> m_functions;
  Function *m_current_function;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
