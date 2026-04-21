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
#include <codegen/handle.hpp>
#include <codegen/parameter.hpp>
#include <codegen/types.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct ExpressionGenerator {
  Handle generate(const zir::ExprId &);
  Handle generate(const zir::ExprKind &);
  Handle generate(const zir::Expression &);

  Handle operator()(const zir::IdentifierExpr &);
  Handle operator()(const zir::Unit &);
  Handle operator()(const zir::I8 &);
  Handle operator()(const zir::I32 &);
  Handle operator()(const zir::I64 &);
  Handle operator()(const zir::Bool &);
  Handle operator()(const zir::Char &);

  [[nodiscard]] zir::TypeId get_block_type(const zir::Block &) const;
  Handle operator()(const zir::Block &);
  Handle operator()(const zir::IfExpr &);
  Handle operator()(const zir::CallExpr &);

  Handle generate_integer_compare_instruction(const zir::BinaryExpr &);
  Handle generate_arithmetic_binary_instruction(const zir::BinaryExpr &);
  Handle generate_logical_binary_instruction(const zir::BinaryExpr &);
  Handle operator()(const zir::BinaryExpr &);

  Handle operator()(const zir::UnaryExpr &);
  Handle operator()(const zir::ReturnExpr &);
  Handle operator()(const zir::CastExpr &);

  FunctionDeclaration generate_lambda_signature(const zir::LambdaExpr &);

  std::pair<TypeId, std::vector<Argument>>
  analyze_captures(const zir::LambdaExpr &);
  Handle operator()(const zir::LambdaExpr &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
