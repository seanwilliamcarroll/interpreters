//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of top-level item generator.
//*
//*
//****************************************************************************

#include "codegen/top_item_generator.hpp"
#include "codegen/expression_generator.hpp"
#include "exceptions.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

void TopItemGenerator::operator()(const hir::FunctionDef &function_def) {
  if (!function_def.m_parameters.empty()) {
    throw core::CompilerException("Codegen", "UNIMPLEMENTED",
                                  function_def.m_location);
  }

  m_ctx.m_output += "define " + function_def.m_type->m_return_type + " @" +
                    function_def.m_function_id + "() {\n";

  auto return_value = ExpressionGenerator{m_ctx}(function_def.m_body);

  m_ctx.m_output += "  ret " + function_def.m_type->m_return_type + " " +
                    return_value.m_reference + "\n";

  m_ctx.m_output += "}\n";
}

void TopItemGenerator::operator()(const hir::LetBinding & /*binding*/) {
  // Stub.
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
