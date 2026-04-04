//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <optional>
#include <source_location.hpp>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************

enum class UnaryOperator : uint8_t {
  // Logic/Bits
  NOT,
  // Arithmetic
  MINUS,
};

enum class BinaryOperator : uint8_t {
  // Logic/Bits
  LOGICAL_AND,
  LOGICAL_OR,
  // BITWISE_AND, // Maybe
  // BITWISE_OR, // maybe
  // Arithmetic
  PLUS,
  MINUS,
  MULTIPLIES,
  DIVIDES,
  MODULUS,
  // Assignment
  // ASSIGNS, // Maybe at some point?
  // Comparison
  EQ,
  LT,
  LT_EQ,
  GT,
  GT_EQ,
  NOT_EQ,
  // DOT, // TODO
};

struct HasLocation {
  core::SourceLocation m_location;
};

struct FunctionDef;
struct LetBinding;

struct Identifier;
struct TypeIdentifierTemp;

struct CallExpr;
struct BinaryExpr;
struct UnaryExpr;

struct IfExpr;
struct Block;
struct ReturnExpr;
struct LambdaExpr;
// TODO
struct WhileExpr : public HasLocation {};
struct ForExpr : public HasLocation {};

template <typename LiteralType> struct AbstractLiteral : public HasLocation {
  LiteralType m_value;
};

using LiteralInt64 = AbstractLiteral<int64_t>;
using LiteralBool = AbstractLiteral<bool>;
struct LiteralUnit : public HasLocation {};

// Those that are recursive should be pointers
using Expression =
    std::variant<Identifier, std::unique_ptr<CallExpr>,
                 std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>,
                 std::unique_ptr<IfExpr>, std::unique_ptr<Block>,
                 std::unique_ptr<ReturnExpr>, std::unique_ptr<LambdaExpr>,
                 std::unique_ptr<WhileExpr>, std::unique_ptr<ForExpr>,
                 LiteralInt64, LiteralBool, LiteralUnit>;

using TopItem =
    std::variant<std::unique_ptr<FunctionDef>, std::unique_ptr<LetBinding>>;

using Statement = std::variant<std::unique_ptr<LetBinding>, Expression>;

enum class PrimitiveType : uint8_t {
  UNIT,
  BOOL,
  INT64,
};

struct PrimitiveTypeIdentifier : public HasLocation {
  PrimitiveType m_type;
};

struct DefinedType : public HasLocation {
  std::string m_type;
};

using TypeIdentifier = std::variant<PrimitiveTypeIdentifier, DefinedType>;

struct Identifier : public HasLocation {
  std::string m_name;
  std::optional<TypeIdentifier> m_type;
};

struct Block : public HasLocation {
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

struct LetBinding : public HasLocation {
  Identifier m_variable;
  Expression m_expression;
};

struct FunctionDef : public HasLocation {
  Identifier m_id;
  std::vector<Identifier> m_parameters;
  TypeIdentifier m_return_type;
  Block m_body;
};

struct LambdaExpr : public HasLocation {
  std::vector<Identifier> m_parameters;
  std::optional<TypeIdentifier> m_return_type;
  Block m_body;
};

// I think it makes sense that we can't have a Program of Programs
struct Program : public HasLocation {
  std::vector<TopItem> m_items;
};

struct IfExpr : public HasLocation {
  Expression m_condition;
  Block m_then_block;
  std::optional<Block> m_else_block;
};

struct ReturnExpr : public HasLocation {
  Expression m_return_expression;
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

//****************************************************************************
} // namespace bust
//****************************************************************************
