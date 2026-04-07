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

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <string>
#include <string_view>
#include <validate_main.hpp>
#include <variant>
#include <vector>

#include "ast/types.hpp"
#include "types.hpp"

//****************************************************************************
namespace bust {
using namespace ast;
//****************************************************************************

bool try_validate_main(const FunctionDef &function_def) {
  if (function_def.m_id.m_name != "main") {
    return false;
  }

  if (!std::holds_alternative<PrimitiveTypeIdentifier>(
          function_def.m_return_type)) {
    throw core::CompilerException(
        "ValidateMain",
        "main function can only return i64 type, not" +
            type_identifier_to_string(function_def.m_return_type),
        function_def.m_location);
  }

  auto primitive_type =
      std::get<PrimitiveTypeIdentifier>(function_def.m_return_type);
  if (primitive_type.m_type != PrimitiveType::I64) {
    throw core::CompilerException(
        "ValidateMain",
        "main function can only return i64 type, not" +
            type_identifier_to_string(function_def.m_return_type),
        function_def.m_location);
  }

  return true;
}

Program ValidateMain::operator()(Program program) const {
  // TODO: validate that:
  // 1. Exactly one FunctionDef named "main" exists
  // 2. main's return type is i64
  // Throw core::CompilerException("SemanticError", ...) on failure

  bool found_main = false;
  for (const auto &top_level_item : program.m_items) {
    if (!std::holds_alternative<FunctionDef>(top_level_item)) {
      continue;
    }
    bool is_valid_main =
        try_validate_main(std::get<FunctionDef>(top_level_item));

    if (is_valid_main) {
      if (found_main) {
        throw core::CompilerException(
            "ValidateMain", "Found second main function!", program.m_location);
      }
      found_main = true;
    }
  }
  if (!found_main) {
    throw core::CompilerException(
        "ValidateMain",
        "Could not find function with identifier \"main\"! This is required "
        "for a bust program!",
        program.m_location);
  }

  return program;
}

//****************************************************************************
} // namespace bust
//****************************************************************************
