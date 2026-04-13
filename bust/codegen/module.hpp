//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Module representation for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/function.hpp>
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

  void add_extern_function_declaration(
      std::unique_ptr<FunctionDeclaration> func_declaration) {
    m_extern_functions.emplace_back(std::move(func_declaration));
  }

  const std::vector<std::unique_ptr<FunctionDeclaration>> &
  extern_functions() const {
    return m_extern_functions;
  }

private:
  std::vector<Global> m_globals;
  std::vector<std::unique_ptr<Function>> m_functions;
  std::vector<std::unique_ptr<FunctionDeclaration>> m_extern_functions;
  Function *m_current_function = nullptr;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
