//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::TypeChecker — char and integer-width
//*            types (i8/i32/i64), cast expressions, extern function
//*            declarations, instantiation records, and callee-as-type-
//*            variable cases.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <hir/dump.hpp>
#include <hir/nodes.hpp>
#include <hir/type_unifier.hpp>
#include <hir/types.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>

#include <algorithm>
#include <ranges>
#include <sstream>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.type_checker.types") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program type_check(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{});
  }

#define DUMP_HIR(program) INFO("HIR:\n" << hir::Dumper::dump(program))

  // --- Char literal type -----------------------------------------------------

  TEST_CASE("literal char has type char") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let c: char = 'A';\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("char literal inferred without annotation") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let c = 'z';\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("char literal with escape has type char") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let c = '\\n';\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("char assigned to i64 binding throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i64 = 'A';\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  // --- New integer type annotations ------------------------------------------

  TEST_CASE("function with i8 return type") {
    auto hir = type_check("fn to_i8() -> i8 { 42 as i8 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &ret = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(std::get<hir::FunctionType>(
                                 hir.m_type_arena.get(func.m_signature.m_type))
                                 .m_return_type));
    CHECK(ret.m_type == PrimitiveType::I8);
  }

  TEST_CASE("function with i32 return type") {
    auto hir = type_check("fn to_i32() -> i32 { 42 as i32 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &ret = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(std::get<hir::FunctionType>(
                                 hir.m_type_arena.get(func.m_signature.m_type))
                                 .m_return_type));
    CHECK(ret.m_type == PrimitiveType::I32);
  }

  TEST_CASE("function with i8 parameter") {
    auto hir = type_check("fn use_i8(x: i8) -> i8 { x }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    auto &p_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_signature.m_parameters[0].m_id.m_type));
    CHECK(p_type.m_type == PrimitiveType::I8);
  }

  TEST_CASE("i8 and i64 are not interchangeable") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i8 = 42;\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("i32 and i64 are not interchangeable") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i32 = 42;\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Cast expressions ------------------------------------------------------

  TEST_CASE("cast i64 to i8 typechecks") {
    auto hir = type_check("fn main() -> i64 { let a: i8 = 42 as i8; 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::I8);
  }

  TEST_CASE("cast i64 to i32 typechecks") {
    auto hir = type_check("fn main() -> i64 { let a: i32 = 42 as i32; 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::I32);
  }

  TEST_CASE("cast i8 to i64 typechecks (widening)") {
    auto hir = type_check("fn widen(x: i8) -> i64 { x as i64 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast i8 to i32 typechecks (widening)") {
    auto hir = type_check("fn widen(x: i8) -> i32 { x as i32 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("cast i32 to i64 typechecks (widening)") {
    auto hir = type_check("fn widen(x: i32) -> i64 { x as i64 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast char to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { 'A' as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast i64 to char typechecks") {
    auto hir = type_check("fn main() -> i64 { let a = 65 as char; 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("cast bool to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { true as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("identity cast i64 to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { 42 as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("identity cast char to char typechecks") {
    auto hir = type_check("fn main() -> i64 { let a = 'A' as char; 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::CHAR);
  }

  // --- Illegal casts ---------------------------------------------------------

  TEST_CASE("cast i64 to bool throws") {
    CHECK_THROWS_AS(type_check("fn main() -> bool { 42 as bool }"),
                    core::CompilerException);
  }

  TEST_CASE("cast char to bool throws") {
    CHECK_THROWS_AS(type_check("fn main() -> bool { 'A' as bool }"),
                    core::CompilerException);
  }

  TEST_CASE("cast bool to char throws") {
    CHECK_THROWS_AS(type_check("fn main() -> char { true as char }"),
                    core::CompilerException);
  }

  TEST_CASE("cast unit to i64 throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { () as i64 }"),
                    core::CompilerException);
  }

  TEST_CASE("cast i64 to unit throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 42 as ();\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Chained casts ---------------------------------------------------------

  TEST_CASE("chained cast i64 to i32 to i8 typechecks") {
    auto hir =
        type_check("fn main() -> i64 { let a: i8 = 42 as i32 as i8; 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::I8);
  }

  TEST_CASE("chained cast char to i32 to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { 'A' as i32 as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Cast in expressions ---------------------------------------------------

  TEST_CASE("cast result used in arithmetic") {
    auto hir = type_check("fn main() -> i64 { 'A' as i64 + 1 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast in let binding with annotation") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: i8 = 42 as i8;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        (hir.m_type_arena.get(let.m_variable.m_type)));
    CHECK(var_type.m_type == PrimitiveType::I8);
  }

  TEST_CASE("cast result type mismatches let annotation throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i32 = 42 as i8;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("arithmetic on mismatched types without cast throws") {
    CHECK_THROWS_AS(type_check("fn add_mixed(a: i8, b: i64) -> i64 {\n"
                               "  a + b\n"
                               "}\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("arithmetic on mismatched types with cast succeeds") {
    auto hir = type_check("fn add_mixed(a: i8, b: i64) -> i64 {\n"
                          "  a as i64 + b\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Arithmetic and comparison on non-i64 integer types -------------------

  TEST_CASE("i8 arithmetic has type i8") {
    auto hir = type_check("fn f(a: i8, b: i8) -> i8 { a + b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("i32 arithmetic has type i32") {
    auto hir = type_check("fn f(a: i32, b: i32) -> i32 { a + b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("i8 subtraction has type i8") {
    auto hir = type_check("fn f(a: i8, b: i8) -> i8 { a - b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("i32 multiplication has type i32") {
    auto hir = type_check("fn f(a: i32, b: i32) -> i32 { a * b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("i8 comparison returns bool") {
    auto hir = type_check("fn f(a: i8, b: i8) -> bool { a < b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("i32 comparison returns bool") {
    auto hir = type_check("fn f(a: i32, b: i32) -> bool { a >= b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("i8 equality returns bool") {
    auto hir = type_check("fn f(a: i8, b: i8) -> bool { a == b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("i8 and i32 arithmetic throws (no implicit coercion)") {
    CHECK_THROWS_AS(type_check("fn f(a: i8, b: i32) -> i32 { a + b }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("i8 and i32 comparison throws") {
    CHECK_THROWS_AS(type_check("fn f(a: i8, b: i32) -> bool { a < b }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("char arithmetic throws") {
    CHECK_THROWS_AS(type_check("fn f(a: char, b: char) -> char { a + b }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("unary minus on i8 produces i8") {
    auto hir = type_check("fn f(a: i8) -> i8 { -a }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("unary minus on i32 produces i32") {
    auto hir = type_check("fn f(a: i32) -> i32 { -a }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("unary minus on char throws") {
    CHECK_THROWS_AS(type_check("fn f(a: char) -> char { -a }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  // --- Extern function declarations -----------------------------------------

  TEST_CASE("extern function declaration produces correct function type") {
    auto hir = type_check("extern fn putchar(c: i32) -> i32;\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
    auto &ext = std::get<hir::ExternFunctionDeclaration>(hir.m_top_items[0]);
    CHECK(ext.m_signature.m_function_id == "putchar");

    auto &fn_type = std::get<hir::FunctionType>(
        hir.m_type_arena.get(ext.m_signature.m_type));
    REQUIRE(fn_type.m_parameters.size() == 1);

    auto &param_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(fn_type.m_parameters[0]));
    CHECK(param_type.m_type == PrimitiveType::I32);

    auto &ret_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(fn_type.m_return_type));
    CHECK(ret_type.m_type == PrimitiveType::I32);
  }

  TEST_CASE(
      "extern function declaration with no return type defaults to unit") {
    auto hir = type_check("extern fn log(x: i64);\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &ext = std::get<hir::ExternFunctionDeclaration>(hir.m_top_items[0]);

    auto &fn_type = std::get<hir::FunctionType>(
        hir.m_type_arena.get(ext.m_signature.m_type));
    auto &ret_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(fn_type.m_return_type));
    CHECK(ret_type.m_type == PrimitiveType::UNIT);
  }

  TEST_CASE("calling extern function type checks arguments") {
    auto hir = type_check("extern fn putchar(c: i32) -> i32;\n"
                          "fn main() -> i64 {\n"
                          "  putchar(72 as i32);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    CHECK(func.m_signature.m_function_id == "main");
  }

  TEST_CASE("calling extern function with wrong argument type throws") {
    CHECK_THROWS_AS(type_check("extern fn putchar(c: i32) -> i32;\n"
                               "fn main() -> i64 {\n"
                               "  putchar(true);\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Instantiation records (for monomorphization) -----------------------

  // Helper: returns the PrimitiveType that a TypeId resolves to.
  // Fails the test if the TypeId isn't a primitive.
  static PrimitiveType resolved_primitive(const hir::Program &hir,
                                          hir::TypeId id) {
    const auto &kind = hir.m_type_arena.get(id);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(kind));
    return std::get<hir::PrimitiveTypeValue>(kind).m_type;
  }

  // Helper: finds the BindingId of a top-level let inside the main function
  // by name. Fails the test if no such binding exists.
  static hir::BindingId binding_id_of(const hir::Program &hir,
                                      const std::string &name) {
    for (const auto &top : hir.m_top_items) {
      const auto *fn = std::get_if<hir::FunctionDef>(&top);
      if (fn == nullptr) {
        continue;
      }
      for (const auto &stmt : fn->m_body.m_statements) {
        const auto *lb = std::get_if<hir::LetBinding>(&stmt);
        if (lb == nullptr) {
          continue;
        }
        if (lb->m_variable.m_name == name) {
          return lb->m_variable.m_id;
        }
      }
    }
    FAIL("no let binding named " << name);
    return hir::BindingId{0};
  }

  TEST_CASE("instantiation records: single polymorphic use yields one record") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let id = |x| { x };\n"
                          "  id(42)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_instantiation_records.size() == 1);
    const auto &[id, records] = *hir.m_instantiation_records.begin();
    CHECK(id == binding_id_of(hir, "id"));
    REQUIRE(records[0].m_substitution.size() == 1);
    const auto &[_, resolved] = *records[0].m_substitution.begin();
    CHECK(resolved_primitive(hir, resolved) == PrimitiveType::I64);
  }

  TEST_CASE("instantiation records: two distinct uses yield two records") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let id = |x| { x };\n"
                          "  let a: i64 = id(42);\n"
                          "  let b: bool = id(true);\n"
                          "  a\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_instantiation_records.size() == 1);
    const auto id_binding = binding_id_of(hir, "id");
    std::vector<PrimitiveType> resolved_types;
    for (const auto &[id, records] : hir.m_instantiation_records) {
      REQUIRE(records.size() == 2);
      for (const auto &record : records) {
        CHECK(id == id_binding);
        REQUIRE(record.m_substitution.size() == 1);
        const auto &[_, resolved] = *record.m_substitution.begin();
        resolved_types.push_back(resolved_primitive(hir, resolved));
      }
    }
    std::ranges::sort(resolved_types);
    REQUIRE(resolved_types.size() == 2);
    CHECK(resolved_types[0] == PrimitiveType::BOOL);
    CHECK(resolved_types[1] == PrimitiveType::I64);
  }

  TEST_CASE("instantiation records: duplicate uses at same type deduped") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let id = |x| { x };\n"
                          "  id(1);\n"
                          "  id(2)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_instantiation_records.size() == 1);
    const auto &[id, records] = *hir.m_instantiation_records.begin();
    CHECK(id == binding_id_of(hir, "id"));
    REQUIRE(records[0].m_substitution.size() == 1);
    const auto &[_, resolved] = *records[0].m_substitution.begin();
    CHECK(resolved_primitive(hir, resolved) == PrimitiveType::I64);
  }

  TEST_CASE("instantiation records: non-polymorphic let yields no records") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x: i64| { x };\n"
                          "  f(1)\n"
                          "}");
    DUMP_HIR(hir);
    CHECK(hir.m_instantiation_records.empty());
  }

  TEST_CASE(
      "instantiation records: nested polymorphic functions both recorded") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let id = |x| { x };\n"
                          "  let apply_id = |y| { id(y) };\n"
                          "  apply_id(42)\n"
                          "}");
    DUMP_HIR(hir);
    std::vector<hir::BindingId> ids;
    for (const auto &[id, records] : hir.m_instantiation_records) {
      ids.push_back(id);
    }
    CHECK(std::ranges::find(ids, binding_id_of(hir, "id")) != ids.end());
    CHECK(std::ranges::find(ids, binding_id_of(hir, "apply_id")) != ids.end());
  }

  // --- Callee-is-a-type-variable at CallExpr ------------------------------
  //
  // These cases exercise the checker's ability to call something whose type
  // hasn't been pinned to a FunctionType yet — typically a lambda parameter
  // used as a callable inside the body. The checker must manufacture a
  // function-shaped constraint and unify, rather than demanding the callee
  // already look like a function.

  TEST_CASE(
      "higher-order: lambda parameter called with one argument type-checks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let apply = |f, x| { f(x) };\n"
                          "  let dbl = |y: i64| { y + y };\n"
                          "  apply(dbl, 21)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(resolved_primitive(hir, func.m_body.m_final_expression->m_type) ==
          PrimitiveType::I64);
  }

  TEST_CASE(
      "higher-order: lambda parameter called with zero arguments type-checks") {
    // Thunk pattern: f has no args; its return type is what call-site uses.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let run = |f| { f() };\n"
                          "  let forty_two = || { 42 };\n"
                          "  run(forty_two)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(resolved_primitive(hir, func.m_body.m_final_expression->m_type) ==
          PrimitiveType::I64);
  }

  TEST_CASE("higher-order: lambda parameter called with multiple arguments "
            "type-checks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let apply2 = |f, x, y| { f(x, y) };\n"
                          "  let add = |a: i64, b: i64| { a + b };\n"
                          "  apply2(add, 3, 4)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(resolved_primitive(hir, func.m_body.m_final_expression->m_type) ==
          PrimitiveType::I64);
  }

  TEST_CASE(
      "higher-order: return type of callee-var propagates through arithmetic") {
    // f(x) + 1 forces the fresh return-type var of f's minted function type
    // to unify with i64 (via the + operator's NUMERIC constraint).
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let plus_one = |f, x| { f(x) + 1 };\n"
                          "  let identity = |v: i64| { v };\n"
                          "  plus_one(identity, 41)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(resolved_primitive(hir, func.m_body.m_final_expression->m_type) ==
          PrimitiveType::I64);
  }

  TEST_CASE(
      "higher-order: nested calls through two variable callees type-check") {
    // Both f and g are parameters — neither is known to be a function before
    // the call site. The inner call f(g(x)) exercises two nested "mint a
    // function type and unify" steps, and couples them: g's return type
    // must unify with f's parameter type.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let compose_apply = |f, g, x| { f(g(x)) };\n"
                          "  let inc = |n: i64| { n + 1 };\n"
                          "  let dbl = |n: i64| { n + n };\n"
                          "  compose_apply(inc, dbl, 10)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(resolved_primitive(hir, func.m_body.m_final_expression->m_type) ==
          PrimitiveType::I64);
  }

  TEST_CASE(
      "higher-order: apply parameter's resolved type is a function type") {
    // The `f` parameter of `apply` should, after type checking, be observable
    // as a FunctionType via the unifier — not just a lingering type variable.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let apply = |f, x| { f(x) };\n"
                          "  let dbl = |y: i64| { y + y };\n"
                          "  apply(dbl, 21)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    // Find apply in the block.
    const hir::LetBinding *apply_let = nullptr;
    for (const auto &stmt : func.m_body.m_statements) {
      if (const auto *let = std::get_if<hir::LetBinding>(&stmt)) {
        if (let->m_variable.m_name == "apply") {
          apply_let = let;
        }
      }
    }
    REQUIRE(apply_let != nullptr);
    const auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
        apply_let->m_expression.m_expression);
    REQUIRE(lambda->m_parameters.size() == 2);
    // Parameter types still carry the post-checker (pre-zir lowering) type ids.
    // The substituter can resolve these via the unifier; asking the
    // arena directly gives us the raw node. That should still be a
    // FunctionType once the checker's work has settled — or, if the AST
    // retains the original type-var, at least not be outright broken.
    // We assert that after resolving through the program's unifier state,
    // the f parameter is either a FunctionType or a type variable whose
    // union-find root resolves to a FunctionType.
    hir::TypeUnifier unifier{hir.m_type_arena};
    unifier.adopt_state(std::move(hir.m_unifier_state));
    auto resolved = unifier.find(lambda->m_parameters[0].m_id.m_type);
    const auto &kind = hir.m_type_arena.get(resolved);
    CHECK(std::holds_alternative<hir::FunctionType>(kind));
  }

  TEST_CASE("higher-order: argument-type constraint propagates to callee-var") {
    // Passing an i64 literal as the argument to a callee-var should pin
    // the minted function type's parameter slot to i64, so that using the
    // same parameter as an i64 elsewhere in the body type-checks.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let use_twice = |f, x| { f(x) + x };\n"
                          "  let id = |v: i64| { v };\n"
                          "  use_twice(id, 7)\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(resolved_primitive(hir, func.m_body.m_final_expression->m_type) ==
          PrimitiveType::I64);
  }

  // --- Negative cases (preserved behavior) --------------------------------

  TEST_CASE(
      "calling a literal throws: callee cannot be a non-function concrete") {
    // Even with the callee-var handling in place, calling a known non-function
    // concrete (an i64 literal) must still produce a type error.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  (42)(1)\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("calling a bool throws: callee cannot be a non-function concrete") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  (true)(1)\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("callee-var called at inconsistent arities fails to unify") {
    // Inside the body, f is first used as a 1-arg function, then as a
    // 2-arg function — those two constraints should conflict via the
    // unifier.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let bad = |f, x| { f(x); f(x, x) };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE(
      "callee-var called with conflicting argument types fails to unify") {
    // f(x) constrains f's param to x's type; f(true) in the same scope
    // constrains it to bool. If x is also used numerically later, conflict.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let bad = |f, x| { f(x); f(true); x + 1 };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
