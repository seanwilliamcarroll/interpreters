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
#include <codegen/value.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct ExpressionGenerator {
  Value generate(const zir::ExprId &);
  Value generate(const zir::Expression &);

  Value operator()(const zir::IdentifierExpr &, zir::TypeId);
  Value operator()(const zir::TupleExpr &, zir::TypeId);
  Value operator()(const zir::Unit &, zir::TypeId);
  Value operator()(const zir::I8 &, zir::TypeId);
  Value operator()(const zir::I32 &, zir::TypeId);
  Value operator()(const zir::I64 &, zir::TypeId);
  Value operator()(const zir::Bool &, zir::TypeId);
  Value operator()(const zir::Char &, zir::TypeId);

  Value generate(const zir::Block &);
  Value operator()(const zir::Block &);
  Value operator()(const zir::IfExpr &, zir::TypeId);

  Value call_lambda_expression(const zir::CallExpr &);
  Value operator()(const zir::CallExpr &, zir::TypeId);

  Value generate_integer_compare_instruction(const zir::BinaryExpr &);
  Value generate_arithmetic_binary_instruction(const zir::BinaryExpr &);
  Value generate_logical_binary_instruction(const zir::BinaryExpr &);
  Value operator()(const zir::BinaryExpr &, zir::TypeId);

  Value operator()(const zir::UnaryExpr &, zir::TypeId);
  Value operator()(const zir::ReturnExpr &, zir::TypeId);
  Value operator()(const zir::CastExpr &, zir::TypeId);

  FunctionDeclaration generate_lambda_signature(const zir::LambdaExpr &,
                                                bool has_env);

  Value lift_free_lambda(const zir::LambdaExpr &);
  Value operator()(const zir::LambdaExpr &, zir::TypeId);

  Value operator()(const zir::DotExpr &, zir::TypeId);
  Value operator()(const zir::WhileExpr &, zir::TypeId);
  Value operator()(const zir::LoopExpr &, zir::TypeId);
  Value operator()(const zir::BreakExpr &, zir::TypeId);
  Value operator()(const zir::ContinueExpr &, zir::TypeId);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
