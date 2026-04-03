//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of the ValidateMain semantic pass.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <validate_main.hpp>

//****************************************************************************
namespace bust {
//****************************************************************************

Program ValidateMain::operator()(Program program) const {
  // TODO: validate that:
  // 1. Exactly one FunctionDef named "main" exists
  // 2. main's return type is i64
  // Throw core::CompilerException("SemanticError", ...) on failure
  return program;
}

//****************************************************************************
} // namespace bust
//****************************************************************************
