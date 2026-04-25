//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression generator for bust LLVM IR codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/context.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct ExpressionGenerator {
  Value generate(const zir::ExprId &);
  Value generate(const zir::ExprKind &);
  Value generate(const zir::Expression &);

  Value operator()(const zir::IdentifierExpr &);
  Value operator()(const zir::TupleExpr &);
  Value operator()(const zir::Unit &);
  Value operator()(const zir::I8 &);
  Value operator()(const zir::I32 &);
  Value operator()(const zir::I64 &);
  Value operator()(const zir::Bool &);
  Value operator()(const zir::Char &);

  Value operator()(const zir::Block &);
  Value operator()(const zir::IfExpr &);

  Value call_lambda_expression(const zir::CallExpr &);
  Value operator()(const zir::CallExpr &);

  Value generate_integer_compare_instruction(const zir::BinaryExpr &);
  Value generate_arithmetic_binary_instruction(const zir::BinaryExpr &);
  Value generate_logical_binary_instruction(const zir::BinaryExpr &);
  Value operator()(const zir::BinaryExpr &);

  Value operator()(const zir::UnaryExpr &);
  Value operator()(const zir::ReturnExpr &);
  Value operator()(const zir::CastExpr &);

  FunctionDeclaration generate_lambda_signature(const zir::LambdaExpr &,
                                                bool has_env);

  Value lift_free_lambda(const zir::LambdaExpr &);
  Value operator()(const zir::LambdaExpr &);

  Value operator()(const zir::DotExpr &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
