//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::ZirLowerer
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <hir/nodes.hpp>
#include <lexer.hpp>
#include <monomorpher.hpp>
#include <parser.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>
#include <zir/nodes.hpp>
#include <zir/program.hpp>
#include <zir/types.hpp>
#include <zir_lowerer.hpp>

#include <sstream>
#include <string>
#include <variant>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.zir_lowerer") {

  // --- Helpers -------------------------------------------------------------

  // Full pipeline: parse -> validate -> typecheck -> mono -> zir
  static zir::Program lower_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{},
                        Monomorpher{}, ZirLowerer{});
  }

  // Retrieve the first FunctionDef from the program's top items.
  static const zir::FunctionDef &first_function(const zir::Program &program) {
    return std::get<zir::FunctionDef>(program.m_top_items.at(0));
  }

  // Retrieve the ExprKind stored in the arena for a given ExprId.
  static const zir::ExprKind &expr_kind(const zir::Program &program,
                                        zir::ExprId id) {
    return program.m_arena.get(id).m_expr_kind;
  }

  // Retrieve the TypeId stored in the arena for a given ExprId.
  static zir::TypeId expr_type(const zir::Program &program, zir::ExprId id) {
    return program.m_arena.get(id).m_type_id;
  }

  // Retrieve a Binding from the arena.
  static const zir::Binding &binding(const zir::Program &program,
                                     zir::BindingId id) {
    return program.m_arena.get(id);
  }

  // Retrieve the ZIR Type from the type arena.
  static const zir::Type &type_of(const zir::Program &program, zir::TypeId id) {
    return program.m_arena.get(id);
  }

  // --- Structure preservation ------------------------------------------------

  TEST_CASE("single function produces one top item") {
    auto zir = lower_string("fn main() -> i64 { 0 }");
    REQUIRE(zir.m_top_items.size() == 1);
    CHECK(std::holds_alternative<zir::FunctionDef>(zir.m_top_items[0]));
  }

  TEST_CASE("function def has correct binding name") {
    auto zir = lower_string("fn main() -> i64 { 0 }");
    auto &func = first_function(zir);
    auto &b = binding(zir, func.m_id);
    CHECK(b.m_name == "main");
  }

  TEST_CASE("function def binding has function type") {
    auto zir = lower_string("fn main() -> i64 { 42 }");
    auto &func = first_function(zir);
    auto &b = binding(zir, func.m_id);
    auto &t = type_of(zir, b.m_type);
    REQUIRE(std::holds_alternative<zir::FunctionType>(t));
    auto &fn_type = std::get<zir::FunctionType>(t);
    CHECK(fn_type.m_parameters.empty());
    CHECK(std::holds_alternative<zir::I64Type>(
        type_of(zir, fn_type.m_return_type)));
  }

  TEST_CASE("multiple top items preserve order") {
    auto zir = lower_string("fn foo() -> i64 { 1 } "
                            "fn main() -> i64 { 0 }");
    REQUIRE(zir.m_top_items.size() == 2);
    auto &f0 = std::get<zir::FunctionDef>(zir.m_top_items[0]);
    auto &f1 = std::get<zir::FunctionDef>(zir.m_top_items[1]);
    CHECK(binding(zir, f0.m_id).m_name == "foo");
    CHECK(binding(zir, f1.m_id).m_name == "main");
  }

  // --- Extern function declarations ------------------------------------------

  TEST_CASE("extern function produces ExternFunctionDeclaration") {
    auto zir = lower_string("extern fn putchar(c: i32) -> i32;\n"
                            "fn main() -> i64 { 0 }");
    REQUIRE(zir.m_top_items.size() == 2);
    REQUIRE(std::holds_alternative<zir::ExternFunctionDeclaration>(
        zir.m_top_items[0]));
    auto &ext = std::get<zir::ExternFunctionDeclaration>(zir.m_top_items[0]);
    auto &b = binding(zir, ext.m_id);
    CHECK(b.m_name == "putchar");
  }

  TEST_CASE("extern function has correct function type") {
    auto zir = lower_string("extern fn putchar(c: i32) -> i32;\n"
                            "fn main() -> i64 { 0 }");
    auto &ext = std::get<zir::ExternFunctionDeclaration>(zir.m_top_items[0]);
    auto &b = binding(zir, ext.m_id);
    auto &t = type_of(zir, b.m_type);
    REQUIRE(std::holds_alternative<zir::FunctionType>(t));
    auto &fn_type = std::get<zir::FunctionType>(t);
    REQUIRE(fn_type.m_parameters.size() == 1);
    CHECK(std::holds_alternative<zir::I32Type>(
        type_of(zir, fn_type.m_parameters[0])));
    CHECK(std::holds_alternative<zir::I32Type>(
        type_of(zir, fn_type.m_return_type)));
  }

  // --- Literal expressions ---------------------------------------------------

  TEST_CASE("i64 literal lowered") {
    auto zir = lower_string("fn main() -> i64 { 42 }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::I64>(kind));
    CHECK(std::get<zir::I64>(kind).m_value == 42);
  }

  TEST_CASE("bool literal lowered") {
    auto zir = lower_string("fn main() -> i64 { if true { 1 } else { 0 } }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::IfExpr>(kind));
    auto &if_expr = std::get<zir::IfExpr>(kind);
    auto &cond = expr_kind(zir, if_expr.m_condition);
    REQUIRE(std::holds_alternative<zir::Bool>(cond));
    CHECK(std::get<zir::Bool>(cond).m_value == true);
  }

  TEST_CASE("char literal lowered") {
    auto zir = lower_string("fn main() -> i64 { 'A' as i64 }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    // The final expression is a CastExpr wrapping a Char literal
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::CastExpr>(kind));
    auto &cast = std::get<zir::CastExpr>(kind);
    auto &inner = expr_kind(zir, cast.m_expression);
    REQUIRE(std::holds_alternative<zir::Char>(inner));
    CHECK(std::get<zir::Char>(inner).m_value == 'A');
  }

  TEST_CASE("i8 literal lowered via cast") {
    auto zir = lower_string("fn main() -> i64 { 7 as i8 as i64 }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    // Outer cast: i8 -> i64
    auto &outer = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::CastExpr>(outer));
    auto &outer_cast = std::get<zir::CastExpr>(outer);
    CHECK(std::holds_alternative<zir::I64Type>(
        type_of(zir, outer_cast.m_new_type)));
    // Inner cast: i64 -> i8
    auto &inner = expr_kind(zir, outer_cast.m_expression);
    REQUIRE(std::holds_alternative<zir::CastExpr>(inner));
    auto &inner_cast = std::get<zir::CastExpr>(inner);
    CHECK(std::holds_alternative<zir::I8Type>(
        type_of(zir, inner_cast.m_new_type)));
  }

  TEST_CASE("unit literal from empty function body") {
    auto zir = lower_string("extern fn putchar(c: i32) -> i32;\n"
                            "fn main() -> i64 { putchar(42 as i32); 0 }");
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items[1]);
    // The statement is an expression statement containing a call
    REQUIRE(func.m_body.m_statements.size() == 1);
    CHECK(std::holds_alternative<zir::ExpressionStatement>(
        func.m_body.m_statements[0]));
  }

  // --- Type correctness ------------------------------------------------------

  TEST_CASE("expression types are concrete ZIR types") {
    auto zir = lower_string("fn main() -> i64 { 42 }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto tid = expr_type(zir, func.m_body.m_final_expression.value());
    CHECK(std::holds_alternative<zir::I64Type>(type_of(zir, tid)));
  }

  TEST_CASE("bool expression has BoolType") {
    auto zir = lower_string(
        "fn main() -> i64 { let b = true; if b { 1 } else { 0 } }");
    auto &func = first_function(zir);
    // The let binding RHS should be a Bool with BoolType
    auto &let = std::get<zir::LetBinding>(func.m_body.m_statements[0]);
    auto tid = expr_type(zir, let.m_expression);
    CHECK(std::holds_alternative<zir::BoolType>(type_of(zir, tid)));
  }

  TEST_CASE("primitive type arena IDs are stable") {
    auto zir = lower_string("fn main() -> i64 { 42 }");
    // TypeArena pre-interns all primitives; check they're consistent
    auto &arena = zir.m_arena;
    CHECK(
        std::holds_alternative<zir::UnitType>(arena.get(arena.type().m_unit)));
    CHECK(
        std::holds_alternative<zir::BoolType>(arena.get(arena.type().m_bool)));
    CHECK(
        std::holds_alternative<zir::CharType>(arena.get(arena.type().m_char)));
    CHECK(std::holds_alternative<zir::I8Type>(arena.get(arena.type().m_i8)));
    CHECK(std::holds_alternative<zir::I32Type>(arena.get(arena.type().m_i32)));
    CHECK(std::holds_alternative<zir::I64Type>(arena.get(arena.type().m_i64)));
    CHECK(std::holds_alternative<zir::NeverType>(
        arena.get(arena.type().m_never)));
  }

  // --- Let bindings ----------------------------------------------------------

  TEST_CASE("let binding produces LetBinding statement") {
    auto zir = lower_string("fn main() -> i64 { let x = 42; x }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_statements.size() == 1);
    auto *let = std::get_if<zir::LetBinding>(&func.m_body.m_statements[0]);
    REQUIRE(let != nullptr);
    // The bound variable has name "x"
    CHECK(binding(zir, let->m_identifier).m_name == "x");
    // The RHS is a literal 42
    auto &rhs = expr_kind(zir, let->m_expression);
    REQUIRE(std::holds_alternative<zir::I64>(rhs));
    CHECK(std::get<zir::I64>(rhs).m_value == 42);
  }

  TEST_CASE("let binding type is correct") {
    auto zir = lower_string("fn main() -> i64 { let x = 42; x }");
    auto &func = first_function(zir);
    auto &let = std::get<zir::LetBinding>(func.m_body.m_statements[0]);
    auto &b = binding(zir, let.m_identifier);
    CHECK(std::holds_alternative<zir::I64Type>(type_of(zir, b.m_type)));
  }

  TEST_CASE("multiple let bindings") {
    auto zir = lower_string("fn main() -> i64 { let x = 1; let y = 2; y }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_statements.size() == 2);
    auto &let0 = std::get<zir::LetBinding>(func.m_body.m_statements[0]);
    auto &let1 = std::get<zir::LetBinding>(func.m_body.m_statements[1]);
    CHECK(binding(zir, let0.m_identifier).m_name == "x");
    CHECK(binding(zir, let1.m_identifier).m_name == "y");
  }

  // --- Identifier expressions ------------------------------------------------

  TEST_CASE("identifier references a binding") {
    auto zir = lower_string("fn main() -> i64 { let x = 42; x }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::IdentifierExpr>(kind));
    auto &ident = std::get<zir::IdentifierExpr>(kind);
    CHECK(binding(zir, ident.m_id).m_name == "x");
  }

  // --- Binary expressions ----------------------------------------------------

  TEST_CASE("binary addition is lowered") {
    auto zir = lower_string("fn main() -> i64 { 20 + 22 }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::BinaryExpr>(kind));
    auto &bin = std::get<zir::BinaryExpr>(kind);
    CHECK(bin.m_operator == BinaryOperator::PLUS);
    REQUIRE(std::holds_alternative<zir::I64>(expr_kind(zir, bin.m_lhs)));
    CHECK(std::get<zir::I64>(expr_kind(zir, bin.m_lhs)).m_value == 20);
    REQUIRE(std::holds_alternative<zir::I64>(expr_kind(zir, bin.m_rhs)));
    CHECK(std::get<zir::I64>(expr_kind(zir, bin.m_rhs)).m_value == 22);
  }

  TEST_CASE("binary comparison produces BinaryExpr") {
    auto zir = lower_string("fn main() -> i64 { if 1 < 2 { 1 } else { 0 } }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::IfExpr>(kind));
    auto &if_expr = std::get<zir::IfExpr>(kind);
    auto &cond = expr_kind(zir, if_expr.m_condition);
    REQUIRE(std::holds_alternative<zir::BinaryExpr>(cond));
    CHECK(std::get<zir::BinaryExpr>(cond).m_operator == BinaryOperator::LT);
  }

  // --- Unary expressions -----------------------------------------------------

  TEST_CASE("unary negation is lowered") {
    auto zir = lower_string("fn main() -> i64 { -42 }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::UnaryExpr>(kind));
    auto &unary = std::get<zir::UnaryExpr>(kind);
    CHECK(unary.m_operator == UnaryOperator::MINUS);
    REQUIRE(
        std::holds_alternative<zir::I64>(expr_kind(zir, unary.m_expression)));
    CHECK(std::get<zir::I64>(expr_kind(zir, unary.m_expression)).m_value == 42);
  }

  TEST_CASE("logical not is lowered") {
    auto zir = lower_string("fn main() -> i64 { if !true { 1 } else { 0 } }");
    auto &func = first_function(zir);
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    auto &if_expr = std::get<zir::IfExpr>(kind);
    auto &cond = expr_kind(zir, if_expr.m_condition);
    REQUIRE(std::holds_alternative<zir::UnaryExpr>(cond));
    CHECK(std::get<zir::UnaryExpr>(cond).m_operator == UnaryOperator::NOT);
  }

  // --- If expressions --------------------------------------------------------

  TEST_CASE("if expression is lowered with then and else blocks") {
    auto zir = lower_string("fn main() -> i64 { if true { 1 } else { 2 } }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::IfExpr>(kind));
    auto &if_expr = std::get<zir::IfExpr>(kind);

    // Condition is a Bool
    auto &cond = expr_kind(zir, if_expr.m_condition);
    REQUIRE(std::holds_alternative<zir::Bool>(cond));
    CHECK(std::get<zir::Bool>(cond).m_value == true);

    // Then block has a final expression
    REQUIRE(if_expr.m_then_block.m_final_expression.has_value());
    auto &then_kind =
        expr_kind(zir, if_expr.m_then_block.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::I64>(then_kind));
    CHECK(std::get<zir::I64>(then_kind).m_value == 1);

    // Else block exists and has a final expression
    REQUIRE(if_expr.m_else_block.has_value());
    REQUIRE(if_expr.m_else_block->m_final_expression.has_value());
    auto &else_kind =
        expr_kind(zir, if_expr.m_else_block->m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::I64>(else_kind));
    CHECK(std::get<zir::I64>(else_kind).m_value == 2);
  }

  TEST_CASE("if branches can contain statements") {
    auto zir = lower_string(
        "fn main() -> i64 { if true { let a = 20; let b = 22; a + b } "
        "else { 0 } }");
    auto &func = first_function(zir);
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    auto &if_expr = std::get<zir::IfExpr>(kind);
    CHECK(if_expr.m_then_block.m_statements.size() == 2);
    REQUIRE(if_expr.m_then_block.m_final_expression.has_value());
  }

  TEST_CASE("nested if expressions") {
    auto zir = lower_string("fn main() -> i64 { if true { if false { 1 } else "
                            "{ 2 } } else { 3 } }");
    auto &func = first_function(zir);
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    auto &outer = std::get<zir::IfExpr>(kind);
    // The then block's final expression should itself be an if
    REQUIRE(outer.m_then_block.m_final_expression.has_value());
    auto &inner_kind =
        expr_kind(zir, outer.m_then_block.m_final_expression.value());
    CHECK(std::holds_alternative<zir::IfExpr>(inner_kind));
  }

  // --- Cast expressions ------------------------------------------------------

  TEST_CASE("cast expression is lowered") {
    auto zir = lower_string("fn main() -> i64 { 42 as i64 }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::CastExpr>(kind));
    auto &cast = std::get<zir::CastExpr>(kind);
    CHECK(std::holds_alternative<zir::I64Type>(type_of(zir, cast.m_new_type)));
  }

  TEST_CASE("cast target type matches ZIR type system") {
    auto zir = lower_string("fn main() -> i64 { 42 as i8 as i64 }");
    auto &func = first_function(zir);
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    auto &outer_cast = std::get<zir::CastExpr>(kind);
    CHECK(std::holds_alternative<zir::I64Type>(
        type_of(zir, outer_cast.m_new_type)));

    auto &inner = expr_kind(zir, outer_cast.m_expression);
    auto &inner_cast = std::get<zir::CastExpr>(inner);
    CHECK(std::holds_alternative<zir::I8Type>(
        type_of(zir, inner_cast.m_new_type)));
  }

  // --- Function calls --------------------------------------------------------

  TEST_CASE("call expression is lowered") {
    auto zir = lower_string("fn id(x: i64) -> i64 { x } "
                            "fn main() -> i64 { id(7) }");
    auto &main_func = std::get<zir::FunctionDef>(zir.m_top_items[1]);
    REQUIRE(main_func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, main_func.m_body.m_final_expression.value());
    REQUIRE(std::holds_alternative<zir::CallExpr>(kind));
    auto &call = std::get<zir::CallExpr>(kind);
    // Callee is an identifier
    auto &callee_kind = expr_kind(zir, call.m_callee);
    REQUIRE(std::holds_alternative<zir::IdentifierExpr>(callee_kind));
    CHECK(
        binding(zir, std::get<zir::IdentifierExpr>(callee_kind).m_id).m_name ==
        "id");
    // One argument
    REQUIRE(call.m_arguments.size() == 1);
    auto &arg_kind = expr_kind(zir, call.m_arguments[0]);
    REQUIRE(std::holds_alternative<zir::I64>(arg_kind));
    CHECK(std::get<zir::I64>(arg_kind).m_value == 7);
  }

  TEST_CASE("multi-argument call") {
    auto zir = lower_string("fn add(x: i64, y: i64) -> i64 { x + y } "
                            "fn main() -> i64 { add(20, 22) }");
    auto &main_func = std::get<zir::FunctionDef>(zir.m_top_items[1]);
    auto &kind = expr_kind(zir, main_func.m_body.m_final_expression.value());
    auto &call = std::get<zir::CallExpr>(kind);
    CHECK(call.m_arguments.size() == 2);
  }

  // --- Function parameters ---------------------------------------------------

  TEST_CASE("function parameters are lowered as BindingIds") {
    auto zir = lower_string("fn add(x: i64, y: i64) -> i64 { x + y } "
                            "fn main() -> i64 { 0 }");
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items[0]);
    REQUIRE(func.m_parameters.size() == 2);
    CHECK(binding(zir, func.m_parameters[0]).m_name == "x");
    CHECK(binding(zir, func.m_parameters[1]).m_name == "y");
    CHECK(std::holds_alternative<zir::I64Type>(
        type_of(zir, binding(zir, func.m_parameters[0]).m_type)));
    CHECK(std::holds_alternative<zir::I64Type>(
        type_of(zir, binding(zir, func.m_parameters[1]).m_type)));
  }

  // --- Return expressions ----------------------------------------------------

  TEST_CASE("return expression is lowered") {
    auto zir =
        lower_string("fn early(x: i64) -> i64 { if x < 0 { return 0; } x } "
                     "fn main() -> i64 { 0 }");
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items[0]);
    // The body should contain an if statement, and the if's then block
    // should contain a return expression
    REQUIRE(func.m_body.m_final_expression.has_value());
  }

  // --- Blocks ----------------------------------------------------------------

  TEST_CASE("block with statements and final expression") {
    auto zir = lower_string("fn main() -> i64 { let a = 1; let b = 2; a + b }");
    auto &func = first_function(zir);
    CHECK(func.m_body.m_statements.size() == 2);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    CHECK(std::holds_alternative<zir::BinaryExpr>(kind));
  }

  TEST_CASE("block without final expression (unit result)") {
    auto zir = lower_string("extern fn putchar(c: i32) -> i32;\n"
                            "fn side_effect() { putchar(42 as i32); }\n"
                            "fn main() -> i64 { 0 }");
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items[1]);
    // A function with no explicit return has no final expression
    CHECK(!func.m_body.m_final_expression.has_value());
  }

  // --- Lambda expressions ----------------------------------------------------

  TEST_CASE("lambda expression is lowered") {
    auto zir = lower_string(
        "fn main() -> i64 { let f = |x: i64| -> i64 { x + 1 }; f(41) }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_statements.size() == 1);
    auto &let = std::get<zir::LetBinding>(func.m_body.m_statements[0]);
    auto &rhs = expr_kind(zir, let.m_expression);
    REQUIRE(std::holds_alternative<zir::LambdaExpr>(rhs));
    auto &lambda = std::get<zir::LambdaExpr>(rhs);
    REQUIRE(lambda.m_parameters.size() == 1);
    CHECK(binding(zir, lambda.m_parameters[0].m_id).m_name == "x");
    CHECK(std::holds_alternative<zir::I64Type>(
        type_of(zir, lambda.m_return_type)));
  }

  // --- Arena consistency -----------------------------------------------------

  TEST_CASE("expression IDs are retrievable from arena") {
    auto zir = lower_string("fn main() -> i64 { let x = 42; x }");
    auto &func = first_function(zir);

    // Final expression is retrievable
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto final_id = func.m_body.m_final_expression.value();
    CHECK_NOTHROW(zir.m_arena.get(final_id));

    // Let binding expression is retrievable
    auto &let = std::get<zir::LetBinding>(func.m_body.m_statements[0]);
    CHECK_NOTHROW(zir.m_arena.get(let.m_expression));
    CHECK_NOTHROW(zir.m_arena.get(let.m_identifier));
  }

  TEST_CASE("binding IDs are retrievable from arena") {
    auto zir = lower_string("fn add(x: i64, y: i64) -> i64 { x + y } "
                            "fn main() -> i64 { 0 }");
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items[0]);
    // Function binding
    CHECK_NOTHROW(binding(zir, func.m_id));
    // Parameter bindings
    for (auto param_id : func.m_parameters) {
      CHECK_NOTHROW(binding(zir, param_id));
    }
  }

  // --- Polymorphic programs (mono -> zir) ----------------------------

  TEST_CASE("polymorphic identity used at one type") {
    auto zir = lower_string("fn main() -> i64 {\n"
                            "  let id = |x| { x };\n"
                            "  id(42)\n"
                            "}");
    auto &func = first_function(zir);
    // After monomorphization, the lambda should be specialized
    // The program should lower without errors
    REQUIRE(func.m_body.m_final_expression.has_value());
  }

  TEST_CASE("polymorphic identity used at two types") {
    auto zir = lower_string("fn main() -> i64 {\n"
                            "  let id = |x| { x };\n"
                            "  id(42);\n"
                            "  id(true);\n"
                            "  0\n"
                            "}");
    auto &func = first_function(zir);
    // Two specializations + two call statements + original removed = ≥2 stmts
    CHECK(func.m_body.m_statements.size() >= 2);
    REQUIRE(func.m_body.m_final_expression.has_value());
  }

  TEST_CASE("polymorphic program types are concrete in ZIR") {
    auto zir = lower_string("fn main() -> i64 {\n"
                            "  let id = |x| { x };\n"
                            "  id(42)\n"
                            "}");
    // Walk all top items and verify no TypeVariable-like issues
    // (ZIR type system has no TypeVariable variant by construction)
    for (const auto &top_item : zir.m_top_items) {
      if (auto *fd = std::get_if<zir::FunctionDef>(&top_item)) {
        auto &b = binding(zir, fd->m_id);
        auto &t = type_of(zir, b.m_type);
        // Type should be a FunctionType (not some error placeholder)
        CHECK(std::holds_alternative<zir::FunctionType>(t));
      }
    }
  }

  // --- Complex programs (integration) ----------------------------------------

  TEST_CASE("recursive function lowers without error") {
    auto zir = lower_string("fn fact(n: i64) -> i64 "
                            "{ if n == 0 { 1 } else { n * fact(n - 1) } } "
                            "fn main() -> i64 { fact(5) }");
    REQUIRE(zir.m_top_items.size() == 2);
    auto &fact_func = std::get<zir::FunctionDef>(zir.m_top_items[0]);
    CHECK(binding(zir, fact_func.m_id).m_name == "fact");
    REQUIRE(fact_func.m_parameters.size() == 1);
    CHECK(binding(zir, fact_func.m_parameters[0]).m_name == "n");
  }

  TEST_CASE("program with extern, function def, and call") {
    auto zir = lower_string("extern fn putchar(c: i32) -> i32;\n"
                            "fn greet() -> i32 { putchar(72 as i32) }\n"
                            "fn main() -> i64 { greet(); 0 }");
    REQUIRE(zir.m_top_items.size() == 3);
    CHECK(std::holds_alternative<zir::ExternFunctionDeclaration>(
        zir.m_top_items[0]));
    CHECK(std::holds_alternative<zir::FunctionDef>(zir.m_top_items[1]));
    CHECK(std::holds_alternative<zir::FunctionDef>(zir.m_top_items[2]));
  }

  TEST_CASE("deeply nested expressions lower correctly") {
    auto zir = lower_string(
        "fn main() -> i64 { ((1 + 2) + (3 + 4)) + ((5 + 6) + (7 + 8)) }");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_final_expression.has_value());
    // The outermost expression should be a binary add
    auto &kind = expr_kind(zir, func.m_body.m_final_expression.value());
    CHECK(std::holds_alternative<zir::BinaryExpr>(kind));
  }

  TEST_CASE("expression with let, if, and binary ops") {
    auto zir = lower_string("fn main() -> i64 {\n"
                            "  let x = 10;\n"
                            "  let y = if x > 5 { x * 2 } else { x + 1 };\n"
                            "  y + 1\n"
                            "}");
    auto &func = first_function(zir);
    REQUIRE(func.m_body.m_statements.size() == 2);
    REQUIRE(func.m_body.m_final_expression.has_value());
  }

  // --- Error handling --------------------------------------------------------

  TEST_CASE("lowering program without unifier state throws") {
    // Construct an empty HIR program without unifier state
    hir::Program program{};
    CHECK_THROWS_AS(ZirLowerer{}(std::move(program)),
                    core::InternalCompilerError);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
