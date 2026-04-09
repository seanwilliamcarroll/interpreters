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

#include "codegen/function.hpp"
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

  const std::vector<Global> &globals() const { return m_globals; }

  const std::vector<std::unique_ptr<Function>> &functions() const {
    return m_functions;
  }

  Function &current_function() { return *m_current_function; }

  void set_current_function(Function &function) {
    m_current_function = &function;
  }

private:
  std::vector<Global> m_globals;
  std::vector<std::unique_ptr<Function>> m_functions;
  Function *m_current_function = nullptr;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
