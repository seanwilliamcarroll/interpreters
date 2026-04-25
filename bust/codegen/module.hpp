//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Module representation for the codegen pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/arena.hpp>
#include <codegen/function.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <zir/arena.hpp>

#include <cstddef>
#include <memory>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct Module {

  explicit Module() = default;

  Function &new_function(FunctionDeclaration signature) {
    m_functions.emplace_back(std::make_unique<Function>(std::move(signature)));
    return *m_functions.back();
  }

  [[nodiscard]] const std::vector<std::unique_ptr<Function>> &
  functions() const {
    return m_functions;
  }

  void add_extern_function_declaration(
      std::unique_ptr<FunctionDeclaration> func_declaration) {
    m_extern_functions.emplace_back(std::move(func_declaration));
  }

  [[nodiscard]] const std::vector<std::unique_ptr<FunctionDeclaration>> &
  extern_functions() const {
    return m_extern_functions;
  }

private:
  std::vector<std::unique_ptr<Function>> m_functions;
  std::vector<std::unique_ptr<FunctionDeclaration>> m_extern_functions;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
