//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::Parser — char literals, type
//*            annotations (i8/i32/char), cast expressions, extern
//*            function declarations, tuples, and dot projection.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <ast/dump.hpp>
#include <ast/nodes.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <tokens.hpp>

#include <sstream>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
using namespace ast;
//****************************************************************************
TEST_SUITE("bust.parser.types") {

  // --- Helpers -------------------------------------------------------------

  static Program parse_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return parser.parse();
  }

#define DUMP_AST(prog) INFO(Dumper::dump(prog))

  static const FunctionDef &get_single_func(const Program &program) {
    REQUIRE(program.m_items.size() == 1);
    REQUIRE(std::holds_alternative<FunctionDef>(program.m_items[0]));
    return std::get<FunctionDef>(program.m_items[0]);
  }

  static const Expression &get_final_expr(const Block &block) {
    REQUIRE(block.m_final_expression.has_value());
    return *block.m_final_expression;
  }

  static void check_primitive_type(const TypeIdentifier &type_id,
                                   PrimitiveType expected) {
    REQUIRE(std::holds_alternative<PrimitiveTypeIdentifier>(type_id));
    CHECK(std::get<PrimitiveTypeIdentifier>(type_id).m_type == expected);
  }

  // === Char literals =========================================================

  TEST_CASE("bust::parse_char_literal_printable") {
    auto program = parse_string("fn main() -> char { 'A' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == 'A');
  }

  TEST_CASE("bust::parse_char_literal_digit") {
    auto program = parse_string("fn main() -> char { '7' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '7');
  }

  TEST_CASE("bust::parse_char_literal_space") {
    auto program = parse_string("fn main() -> char { ' ' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == ' ');
  }

  TEST_CASE("bust::parse_char_literal_escape_newline") {
    auto program = parse_string("fn main() -> char { '\\n' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\n');
  }

  TEST_CASE("bust::parse_char_literal_escape_tab") {
    auto program = parse_string("fn main() -> char { '\\t' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\t');
  }

  TEST_CASE("bust::parse_char_literal_escape_null") {
    auto program = parse_string("fn main() -> char { '\\0' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\0');
  }

  TEST_CASE("bust::parse_char_literal_escape_backslash") {
    auto program = parse_string("fn main() -> char { '\\\\' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\\');
  }

  TEST_CASE("bust::parse_char_literal_escape_single_quote") {
    auto program = parse_string("fn main() -> char { '\\'' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\'');
  }

  TEST_CASE("bust::parse_char_literal_hex_escape") {
    // '\x41' is 'A' (0x41 = 65)
    auto program = parse_string("fn main() -> char { '\\x41' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == 'A');
  }

  TEST_CASE("bust::parse_char_literal_hex_escape_null") {
    // '\x00' is null
    auto program = parse_string("fn main() -> char { '\\x00' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\0');
  }

  // === Type annotations (i8, i32, char) ======================================

  TEST_CASE("bust::parse_type_annotation_i8") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i8 = 42;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::I8);
  }

  TEST_CASE("bust::parse_type_annotation_i32") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i32 = 42;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_type_annotation_char") {
    auto program = parse_string("fn main() -> char { 'A' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    check_primitive_type(func.m_signature.m_return_type, PrimitiveType::CHAR);
  }

  TEST_CASE("bust::parse_function_param_i8") {
    auto program = parse_string("fn foo(x: i8) -> i8 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    REQUIRE(func.m_signature.m_parameters[0].m_id.m_type.has_value());
    check_primitive_type(*func.m_signature.m_parameters[0].m_id.m_type,
                         PrimitiveType::I8);
    check_primitive_type(func.m_signature.m_return_type, PrimitiveType::I8);
  }

  // === Cast expressions ======================================================

  TEST_CASE("bust::parse_cast_simple") {
    // 42 as i8
    auto program = parse_string("fn main() -> i8 { 42 as i8 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<I64>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I8);
  }

  TEST_CASE("bust::parse_cast_to_i32") {
    auto program = parse_string("fn main() -> i32 { x as i32 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<Identifier>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_cast_to_char") {
    auto program = parse_string("fn main() -> char { 65 as char }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    check_primitive_type(cast.m_new_type, PrimitiveType::CHAR);
  }

  TEST_CASE("bust::parse_cast_chained") {
    // x as i32 as i64 — left-associative: (x as i32) as i64
    auto program = parse_string("fn main() -> i64 { x as i32 as i64 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &outer = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    check_primitive_type(outer.m_new_type, PrimitiveType::I64);
    // Inner is also a cast
    REQUIRE(std::holds_alternative<std::unique_ptr<CastExpr>>(
        outer.m_expression.m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<CastExpr>>(outer.m_expression.m_expression);
    CHECK(std::holds_alternative<Identifier>(inner.m_expression.m_expression));
    check_primitive_type(inner.m_new_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_cast_lower_precedence_than_arithmetic") {
    // x as i32 + 1 should parse as (x as i32) + 1
    auto program = parse_string("fn main() -> i64 { x as i32 + 1 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &add = *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(add.m_operator == BinaryOperator::PLUS);
    // LHS is the cast
    REQUIRE(std::holds_alternative<std::unique_ptr<CastExpr>>(
        add.m_lhs.m_expression));
    const auto &cast =
        *std::get<std::unique_ptr<CastExpr>>(add.m_lhs.m_expression);
    CHECK(std::holds_alternative<Identifier>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I32);
    // RHS is literal 1
    CHECK(std::holds_alternative<I64>(add.m_rhs.m_expression));
  }

  TEST_CASE("bust::parse_cast_with_unary") {
    // -x as i32 should parse as -(x as i32)
    auto program = parse_string("fn main() -> i32 { -x as i32 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<UnaryExpr>>(expr.m_expression));
    const auto &unary =
        *std::get<std::unique_ptr<UnaryExpr>>(expr.m_expression);
    CHECK(unary.m_operator == UnaryOperator::MINUS);
    REQUIRE(std::holds_alternative<std::unique_ptr<CastExpr>>(
        unary.m_expression.m_expression));
    const auto &cast =
        *std::get<std::unique_ptr<CastExpr>>(unary.m_expression.m_expression);
    CHECK(std::holds_alternative<Identifier>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_cast_on_call_result") {
    // foo() as i8
    auto program = parse_string("fn main() -> i8 { foo() as i8 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<std::unique_ptr<CallExpr>>(
        cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I8);
  }

  // === Extern function declarations =========================================

  static const ExternFunctionDeclaration &get_single_extern(
      const Program &program) {
    REQUIRE(program.m_items.size() == 1);
    REQUIRE(
        std::holds_alternative<ExternFunctionDeclaration>(program.m_items[0]));
    return std::get<ExternFunctionDeclaration>(program.m_items[0]);
  }

  TEST_CASE("bust::parse_extern_function_no_params") {
    auto program = parse_string("extern fn abort() -> i64;");
    DUMP_AST(program);
    const auto &ext = get_single_extern(program);
    CHECK(ext.m_signature.m_id.m_name == "abort");
    CHECK(ext.m_signature.m_parameters.empty());
    check_primitive_type(ext.m_signature.m_return_type, PrimitiveType::I64);
  }

  TEST_CASE("bust::parse_extern_function_with_param") {
    auto program = parse_string("extern fn putchar(c: i32) -> i32;");
    DUMP_AST(program);
    const auto &ext = get_single_extern(program);
    CHECK(ext.m_signature.m_id.m_name == "putchar");
    REQUIRE(ext.m_signature.m_parameters.size() == 1);
    CHECK(ext.m_signature.m_parameters[0].m_id.m_name == "c");
    REQUIRE(ext.m_signature.m_parameters[0].m_id.m_type.has_value());
    check_primitive_type(*ext.m_signature.m_parameters[0].m_id.m_type,
                         PrimitiveType::I32);
    check_primitive_type(ext.m_signature.m_return_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_extern_function_no_return_type") {
    auto program = parse_string("extern fn log_message(msg: i64);");
    DUMP_AST(program);
    const auto &ext = get_single_extern(program);
    CHECK(ext.m_signature.m_id.m_name == "log_message");
    check_primitive_type(ext.m_signature.m_return_type, PrimitiveType::UNIT);
  }

  TEST_CASE("bust::parse_extern_alongside_func_def") {
    auto program = parse_string("extern fn putchar(c: i32) -> i32;\n"
                                "fn main() -> i64 { 0 }");
    DUMP_AST(program);
    REQUIRE(program.m_items.size() == 2);
    REQUIRE(
        std::holds_alternative<ExternFunctionDeclaration>(program.m_items[0]));
    REQUIRE(std::holds_alternative<FunctionDef>(program.m_items[1]));
    const auto &ext = std::get<ExternFunctionDeclaration>(program.m_items[0]);
    CHECK(ext.m_signature.m_id.m_name == "putchar");
    const auto &func = std::get<FunctionDef>(program.m_items[1]);
    CHECK(func.m_signature.m_id.m_name == "main");
  }

  // === Cast expressions ======================================================

  TEST_CASE("bust::parse_cast_in_let_binding") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i8 = 42 as i8;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::I8);
    CHECK(std::holds_alternative<std::unique_ptr<CastExpr>>(
        binding.m_expression.m_expression));
  }

  // === Tuple construction ====================================================

  TEST_CASE("bust::parse_tuple_two_elements") {
    auto program = parse_string("fn main() -> i64 { (1, 2) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &tup = *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(tup.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<I64>(tup.m_fields[0].m_expression));
    CHECK(std::get<I64>(tup.m_fields[0].m_expression).m_value == 1);
    REQUIRE(std::holds_alternative<I64>(tup.m_fields[1].m_expression));
    CHECK(std::get<I64>(tup.m_fields[1].m_expression).m_value == 2);
  }

  TEST_CASE("bust::parse_tuple_trailing_comma") {
    // (1, 2,) is the same tuple as (1, 2).
    auto program = parse_string("fn main() -> i64 { (1, 2,) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &tup = *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    CHECK(tup.m_fields.size() == 2);
  }

  TEST_CASE("bust::parse_one_tuple") {
    // (x,) — trailing comma is required for 1-tuples.
    auto program = parse_string("fn main() -> i64 { (x,) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &tup = *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(tup.m_fields.size() == 1);
    REQUIRE(std::holds_alternative<Identifier>(tup.m_fields[0].m_expression));
    CHECK(std::get<Identifier>(tup.m_fields[0].m_expression).m_name == "x");
  }

  TEST_CASE("bust::parse_nested_tuple_inner_first") {
    // ((1, 2), 3) — first element is itself a 2-tuple.
    auto program = parse_string("fn main() -> i64 { ((1, 2), 3) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &outer =
        *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(outer.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[0].m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[0].m_expression);
    REQUIRE(inner.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<I64>(inner.m_fields[0].m_expression));
    CHECK(std::get<I64>(inner.m_fields[0].m_expression).m_value == 1);
    REQUIRE(std::holds_alternative<I64>(inner.m_fields[1].m_expression));
    CHECK(std::get<I64>(inner.m_fields[1].m_expression).m_value == 2);
    REQUIRE(std::holds_alternative<I64>(outer.m_fields[1].m_expression));
    CHECK(std::get<I64>(outer.m_fields[1].m_expression).m_value == 3);
  }

  TEST_CASE("bust::parse_tuple_of_tuples") {
    // ((1, 2), (3, 4)) — both elements are tuples.
    auto program = parse_string("fn main() -> i64 { ((1, 2), (3, 4)) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &outer =
        *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(outer.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[0].m_expression));
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[1].m_expression));
    const auto &left =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[0].m_expression);
    const auto &right =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[1].m_expression);
    CHECK(left.m_fields.size() == 2);
    CHECK(right.m_fields.size() == 2);
  }

  TEST_CASE("bust::parse_nested_one_tuple") {
    // ((x,),) — 1-tuple containing a 1-tuple; stress-tests trailing commas
    // at multiple depths.
    auto program = parse_string("fn main() -> i64 { ((x,),) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &outer =
        *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(outer.m_fields.size() == 1);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[0].m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[0].m_expression);
    REQUIRE(inner.m_fields.size() == 1);
    REQUIRE(std::holds_alternative<Identifier>(inner.m_fields[0].m_expression));
    CHECK(std::get<Identifier>(inner.m_fields[0].m_expression).m_name == "x");
  }

  TEST_CASE("bust::parse_parenthesized_identifier_is_not_tuple") {
    // (x) is just x — no trailing comma means no tuple.
    auto program = parse_string("fn main() -> i64 { (x) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Identifier>(expr.m_expression));
    CHECK(std::get<Identifier>(expr.m_expression).m_name == "x");
  }

  // === Tuple type annotations ===============================================

  TEST_CASE("bust::parse_tuple_type_in_let") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let t: (i64, bool) = (1, true);\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &tt = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(tt.m_field_types.size() == 2);
    check_primitive_type(tt.m_field_types[0], PrimitiveType::I64);
    check_primitive_type(tt.m_field_types[1], PrimitiveType::BOOL);
  }

  TEST_CASE("bust::parse_tuple_of_tuple_type_in_let") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let t: ((i64,), bool) = ((1,), true);\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &tt = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(tt.m_field_types.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        tt.m_field_types[0]));
    check_primitive_type(
        std::get<std::unique_ptr<TupleTypeIdentifier>>(tt.m_field_types[0])
            ->m_field_types[0],
        PrimitiveType::I64);
    check_primitive_type(tt.m_field_types[1], PrimitiveType::BOOL);
  }

  TEST_CASE("bust::parse_nested_tuple_type_in_let") {
    // ((i64, i64), bool) — nested tuple in the first position.
    auto program =
        parse_string("fn main() -> i64 {\n"
                     "  let t: ((i64, i64), bool) = ((1, 2), true);\n"
                     "  0\n"
                     "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &outer = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(outer.m_field_types.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        outer.m_field_types[0]));
    const auto &inner =
        *std::get<std::unique_ptr<TupleTypeIdentifier>>(outer.m_field_types[0]);
    REQUIRE(inner.m_field_types.size() == 2);
    check_primitive_type(inner.m_field_types[0], PrimitiveType::I64);
    check_primitive_type(inner.m_field_types[1], PrimitiveType::I64);
    check_primitive_type(outer.m_field_types[1], PrimitiveType::BOOL);
  }

  TEST_CASE("bust::parse_deeply_nested_tuple_type") {
    // (i64, (bool, (char, i32))) — three levels of nesting, right-heavy.
    auto program = parse_string(
        "fn main() -> i64 {\n"
        "  let t: (i64, (bool, (char, i32))) = (1, (true, ('a', 2)));\n"
        "  0\n"
        "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    const auto &lvl0 = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(lvl0.m_field_types.size() == 2);
    check_primitive_type(lvl0.m_field_types[0], PrimitiveType::I64);
    const auto &lvl1 =
        *std::get<std::unique_ptr<TupleTypeIdentifier>>(lvl0.m_field_types[1]);
    REQUIRE(lvl1.m_field_types.size() == 2);
    check_primitive_type(lvl1.m_field_types[0], PrimitiveType::BOOL);
    const auto &lvl2 =
        *std::get<std::unique_ptr<TupleTypeIdentifier>>(lvl1.m_field_types[1]);
    REQUIRE(lvl2.m_field_types.size() == 2);
    check_primitive_type(lvl2.m_field_types[0], PrimitiveType::CHAR);
    check_primitive_type(lvl2.m_field_types[1], PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_one_tuple_type") {
    // (i64,) — 1-tuple type requires trailing comma.
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let t: (i64,) = (1,);\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &tt = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(tt.m_field_types.size() == 1);
    check_primitive_type(tt.m_field_types[0], PrimitiveType::I64);
  }

  // === Dot projection =======================================================

  TEST_CASE("bust::parse_dot_projection") {
    auto program = parse_string("fn main() -> i64 { t.0 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<DotExpr>>(expr.m_expression));
    const auto &dot = *std::get<std::unique_ptr<DotExpr>>(expr.m_expression);
    CHECK(dot.m_tuple_index == 0);
    REQUIRE(std::holds_alternative<Identifier>(dot.m_expression.m_expression));
    CHECK(std::get<Identifier>(dot.m_expression.m_expression).m_name == "t");
  }

  TEST_CASE("bust::parse_chained_dot_projection") {
    // t.0.1 parses as (t.0).1 — left-associative.
    auto program = parse_string("fn main() -> i64 { t.0.1 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<DotExpr>>(expr.m_expression));
    const auto &outer = *std::get<std::unique_ptr<DotExpr>>(expr.m_expression);
    CHECK(outer.m_tuple_index == 1);
    REQUIRE(std::holds_alternative<std::unique_ptr<DotExpr>>(
        outer.m_expression.m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<DotExpr>>(outer.m_expression.m_expression);
    CHECK(inner.m_tuple_index == 0);
  }

  TEST_CASE("bust::parse_dot_tighter_than_cast") {
    // t.0 as i32 parses as (t.0) as i32.
    auto program = parse_string("fn main() -> i32 { t.0 as i32 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<std::unique_ptr<DotExpr>>(
        cast.m_expression.m_expression));
  }
}
//****************************************************************************
} // namespace bust
//****************************************************************************
