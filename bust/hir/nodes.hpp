//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Typed AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <operators.hpp>
#include <source_location.hpp>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
using bust::BinaryOperator;
using bust::UnaryOperator;
using core::HasLocation;
//****************************************************************************

enum class PrimitiveTypeEnum : uint8_t {
  UNIT,
  BOOL,
  I64,
};

struct PrimitiveType : public HasLocation {
  PrimitiveTypeEnum m_type;
};

struct FunctionType;

struct NeverType : public HasLocation {};

// TODO: Do we want to have an InferredType and ExplicitType, where Explicit has
// a location?
// TODO: Unknown type?
// TODO: User defined types of some kind
using Type =
    std::variant<PrimitiveType, std::unique_ptr<FunctionType>, NeverType>;

struct FunctionType : public HasLocation {
  // I think this should have a location, inferred types may not
  std::vector<Type> m_argument_types;
  Type m_return_type;
};

struct Identifier : public HasLocation {
  std::string m_name;
  Type m_type;
};

struct LetBinding;
struct Block;
struct IfExpr;
struct CallExpr;
struct BinaryExpr;
struct UnaryExpr;
struct ReturnExpr;
struct LambdaExpr;
// TODO
struct WhileExpr {};
struct ForExpr {};

template <PrimitiveTypeEnum InternalType> struct Literal : public HasLocation {
  static constexpr PrimitiveTypeEnum m_type = InternalType;
};

template <> struct Literal<PrimitiveTypeEnum::BOOL> : public HasLocation {
  static constexpr PrimitiveTypeEnum m_type = PrimitiveTypeEnum::BOOL;
  bool m_value;
};

template <> struct Literal<PrimitiveTypeEnum::I64> : public HasLocation {
  static constexpr PrimitiveTypeEnum m_type = PrimitiveTypeEnum::I64;
  int64_t m_value;
};

using LiteralI64 = Literal<PrimitiveTypeEnum::I64>;
using LiteralBool = Literal<PrimitiveTypeEnum::BOOL>;
using LiteralUnit = Literal<PrimitiveTypeEnum::UNIT>;

using ExprKind =
    std::variant<Identifier, LiteralUnit, LiteralI64, LiteralBool,
                 std::unique_ptr<Block>, std::unique_ptr<IfExpr>,
                 std::unique_ptr<CallExpr>, std::unique_ptr<BinaryExpr>,
                 std::unique_ptr<UnaryExpr>, std::unique_ptr<ReturnExpr>,
                 std::unique_ptr<LambdaExpr>>;

struct Expression : public HasLocation {
  Type m_type;
  ExprKind m_expression;
};

using Statement = std::variant<Expression, std::unique_ptr<LetBinding>>;

struct Block : public HasLocation {
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

struct IfExpr : public HasLocation {
  Expression m_condition;
  Block m_then_branch;
  std::optional<Block> m_else_branch;
};

struct CallExpr : public HasLocation {
  Expression m_callee;
  std::vector<Expression> m_arguments;
};

struct BinaryExpr : public HasLocation {
  BinaryOperator m_operator;
  Expression m_lhs;
  Expression m_rhs;
};

struct UnaryExpr : public HasLocation {
  UnaryOperator m_operator;
  Expression m_expression;
};

struct ReturnExpr : public HasLocation {
  Expression m_expression;
};

struct LambdaExpr : public HasLocation {
  std::vector<Identifier> m_parameters;
  Block m_body;
};

struct FunctionDef : public HasLocation {
  // Would contain a function type?
  std::string m_function_id;
  FunctionType m_type;
  std::vector<Identifier> m_parameters;
  Block m_body;
};

struct LetBinding : public HasLocation {
  Identifier m_variable;
  Expression m_expression;
};

using TopItem = std::variant<FunctionDef, std::unique_ptr<LetBinding>>;

struct Program : public HasLocation {
  std::vector<TopItem> m_top_items;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
