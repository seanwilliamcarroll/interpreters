//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for blip::Parser
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <parser.hpp>

#include <ast.hpp>
#include <blip_tokens.hpp>
#include <lexer.hpp>

#include <doctest/doctest.h> // For doctest
#include <sstream>           // For istringstream

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.parser") {

  // Helper: parse a string and return the AST root
  static std::unique_ptr<AstNode> parse_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return parser.parse();
  }

  // --- Atoms ----------------------------------------------------------------

  TEST_CASE("parse integer literal") {
    auto ast = parse_string("42");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit = dynamic_cast<IntLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == 42);
  }

  TEST_CASE("parse string literal") {
    auto ast = parse_string("\"hello\"");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit = dynamic_cast<StringLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == "hello");
  }

  TEST_CASE("parse double literal") {
    auto ast = parse_string("34.9999");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit = dynamic_cast<DoubleLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == 34.9999);
  }

  TEST_CASE("parse bool literal") {
    auto ast = parse_string("true");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit = dynamic_cast<BoolLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == true);
  }

  TEST_CASE("parse identifier") {
    auto ast = parse_string("foo");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *id = dynamic_cast<Identifier *>(program->get_program()[0].get());
    REQUIRE(id != nullptr);
    CHECK(id->get_name() == "foo");
  }

  // --- Special forms --------------------------------------------------------

  TEST_CASE("parse nonsense program") {
    auto ast = parse_string("42\n54\ntrue\n34.9999\n\"blah blah blah\"");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 5);

    auto *int_0 = dynamic_cast<IntLiteral *>(program->get_program()[0].get());
    REQUIRE(int_0 != nullptr);
    CHECK(int_0->get_value() == 42);

    auto *int_1 = dynamic_cast<IntLiteral *>(program->get_program()[1].get());
    REQUIRE(int_1 != nullptr);
    CHECK(int_1->get_value() == 54);

    auto *bool_0 = dynamic_cast<BoolLiteral *>(program->get_program()[2].get());
    REQUIRE(bool_0 != nullptr);
    CHECK(bool_0->get_value() == true);

    auto *double_0 =
        dynamic_cast<DoubleLiteral *>(program->get_program()[3].get());
    REQUIRE(double_0 != nullptr);
    // Not sure this will work
    CHECK(double_0->get_value() == 34.9999);

    auto *string_0 =
        dynamic_cast<StringLiteral *>(program->get_program()[4].get());
    REQUIRE(string_0 != nullptr);
    CHECK(string_0->get_value() == "blah blah blah");
  }

  TEST_CASE("parse print") {
    auto ast = parse_string("(print 42)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *print = dynamic_cast<PrintNode *>(program->get_program()[0].get());
    REQUIRE(print != nullptr);

    auto *lit = dynamic_cast<const IntLiteral *>(&print->get_expression());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == 42);
  }

  TEST_CASE("parse set") {
    auto ast = parse_string("(set x 10)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *set = dynamic_cast<SetNode *>(program->get_program()[0].get());
    REQUIRE(set != nullptr);
    CHECK(set->get_name().get_name() == "x");

    auto *val = dynamic_cast<const IntLiteral *>(&set->get_value());
    REQUIRE(val != nullptr);
    CHECK(val->get_value() == 10);
  }

  TEST_CASE("parse begin") {
    auto ast = parse_string("(begin 1 2 3)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *begin = dynamic_cast<BeginNode *>(program->get_program()[0].get());
    REQUIRE(begin != nullptr);
    REQUIRE(begin->get_expressions().size() == 3);

    auto *first = dynamic_cast<IntLiteral *>(begin->get_expressions()[0].get());
    REQUIRE(first != nullptr);
    CHECK(first->get_value() == 1);

    auto *last = dynamic_cast<IntLiteral *>(begin->get_expressions()[2].get());
    REQUIRE(last != nullptr);
    CHECK(last->get_value() == 3);
  }

  TEST_CASE("parse if with else") {
    auto ast = parse_string("(if true 1 2)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *if_node = dynamic_cast<IfNode *>(program->get_program()[0].get());
    REQUIRE(if_node != nullptr);

    auto *cond = dynamic_cast<const BoolLiteral *>(&if_node->get_condition());
    REQUIRE(cond != nullptr);
    CHECK(cond->get_value() == true);

    auto *then_br =
        dynamic_cast<const IntLiteral *>(&if_node->get_then_branch());
    REQUIRE(then_br != nullptr);
    CHECK(then_br->get_value() == 1);

    auto *else_br =
        dynamic_cast<const IntLiteral *>(if_node->get_else_branch());
    REQUIRE(else_br != nullptr);
    CHECK(else_br->get_value() == 2);
  }

  TEST_CASE("parse if without else") {
    auto ast = parse_string("(if false 99)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *if_node = dynamic_cast<IfNode *>(program->get_program()[0].get());
    REQUIRE(if_node != nullptr);
    CHECK(if_node->get_else_branch() == nullptr);

    auto *then_br =
        dynamic_cast<const IntLiteral *>(&if_node->get_then_branch());
    REQUIRE(then_br != nullptr);
    CHECK(then_br->get_value() == 99);
  }

  TEST_CASE("parse while") {
    auto ast = parse_string("(while true 42)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *wh = dynamic_cast<WhileNode *>(program->get_program()[0].get());
    REQUIRE(wh != nullptr);

    auto *cond = dynamic_cast<const BoolLiteral *>(&wh->get_condition());
    REQUIRE(cond != nullptr);
    CHECK(cond->get_value() == true);

    auto *body = dynamic_cast<const IntLiteral *>(&wh->get_body());
    REQUIRE(body != nullptr);
    CHECK(body->get_value() == 42);
  }

  TEST_CASE("parse define variable") {
    auto ast = parse_string("(define x 5)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *def = dynamic_cast<DefineVarNode *>(program->get_program()[0].get());
    REQUIRE(def != nullptr);
    CHECK(def->get_name().get_name() == "x");

    auto *val = dynamic_cast<const IntLiteral *>(&def->get_value());
    REQUIRE(val != nullptr);
    CHECK(val->get_value() == 5);
  }

  TEST_CASE("parse define function with type annotations") {
    auto ast =
        parse_string("(define (add (a : int) (b : int)) : int (print a))");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *def = dynamic_cast<DefineFnNode *>(program->get_program()[0].get());
    REQUIRE(def != nullptr);
    CHECK(def->get_name().get_name() == "add");
    REQUIRE(def->get_arguments().size() == 2);
    CHECK(def->get_arguments()[0]->get_name() == "a");
    CHECK(def->get_arguments()[1]->get_name() == "b");

    // Check parameter type annotations
    REQUIRE(def->get_arguments()[0]->get_type() != nullptr);
    CHECK(def->get_arguments()[0]->get_type()->get_type_name() == "int");
    REQUIRE(def->get_arguments()[1]->get_type() != nullptr);
    CHECK(def->get_arguments()[1]->get_type()->get_type_name() == "int");

    // Check return type annotation
    CHECK(def->get_return_type().get_type_name() == "int");

    auto *body = dynamic_cast<const PrintNode *>(&def->get_body());
    REQUIRE(body != nullptr);
  }

  TEST_CASE("parse define function zero params") {
    auto ast = parse_string("(define (f) : unit 42)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *def = dynamic_cast<DefineFnNode *>(program->get_program()[0].get());
    REQUIRE(def != nullptr);
    CHECK(def->get_name().get_name() == "f");
    CHECK(def->get_arguments().size() == 0);

    CHECK(def->get_return_type().get_type_name() == "unit");

    auto *body = dynamic_cast<const IntLiteral *>(&def->get_body());
    REQUIRE(body != nullptr);
    CHECK(body->get_value() == 42);
  }

  TEST_CASE("parse define function with different type names") {
    auto ast = parse_string(
        "(define (greet (name : string) (loud : bool)) : string (print name))");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *def = dynamic_cast<DefineFnNode *>(program->get_program()[0].get());
    REQUIRE(def != nullptr);
    CHECK(def->get_name().get_name() == "greet");
    REQUIRE(def->get_arguments().size() == 2);

    CHECK(def->get_arguments()[0]->get_name() == "name");
    CHECK(def->get_arguments()[0]->get_type()->get_type_name() == "string");

    CHECK(def->get_arguments()[1]->get_name() == "loud");
    CHECK(def->get_arguments()[1]->get_type()->get_type_name() == "bool");

    CHECK(def->get_return_type().get_type_name() == "string");
  }

  TEST_CASE("parse define function missing param type throws") {
    // Bare identifier without (name : type) should fail
    CHECK_THROWS_AS(parse_string("(define (f x) : int 42)"),
                    core::CompilerException);
  }

  TEST_CASE("parse define function missing return type throws") {
    // No : type after param list closing paren
    CHECK_THROWS_AS(parse_string("(define (f (x : int)) 42)"),
                    core::CompilerException);
  }

  // --- Function calls -------------------------------------------------------

  TEST_CASE("parse function call") {
    auto ast = parse_string("(foo 1 2)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *call = dynamic_cast<CallNode *>(program->get_program()[0].get());
    REQUIRE(call != nullptr);

    auto *callee = dynamic_cast<const Identifier *>(&call->get_callee());
    REQUIRE(callee != nullptr);
    CHECK(callee->get_name() == "foo");

    REQUIRE(call->get_arguments().size() == 2);
    auto *arg0 = dynamic_cast<IntLiteral *>(call->get_arguments()[0].get());
    REQUIRE(arg0 != nullptr);
    CHECK(arg0->get_value() == 1);
  }

  TEST_CASE("parse function call no args") {
    auto ast = parse_string("(foo)");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *call = dynamic_cast<CallNode *>(program->get_program()[0].get());
    REQUIRE(call != nullptr);

    auto *callee = dynamic_cast<const Identifier *>(&call->get_callee());
    REQUIRE(callee != nullptr);
    CHECK(callee->get_name() == "foo");
    CHECK(call->get_arguments().size() == 0);
  }

  // --- Nested expressions ---------------------------------------------------

  TEST_CASE("parse nested expression") {
    auto ast = parse_string("(print (if true 1 2))");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *print = dynamic_cast<PrintNode *>(program->get_program()[0].get());
    REQUIRE(print != nullptr);

    auto *if_node = dynamic_cast<const IfNode *>(&print->get_expression());
    REQUIRE(if_node != nullptr);

    auto *then_br =
        dynamic_cast<const IntLiteral *>(&if_node->get_then_branch());
    REQUIRE(then_br != nullptr);
    CHECK(then_br->get_value() == 1);

    auto *else_br =
        dynamic_cast<const IntLiteral *>(if_node->get_else_branch());
    REQUIRE(else_br != nullptr);
    CHECK(else_br->get_value() == 2);
  }

  // --- Error cases ----------------------------------------------------------

  TEST_CASE("begin with no expressions throws") {
    CHECK_THROWS_AS(parse_string("(begin)"), core::CompilerException);
  }

  TEST_CASE("parse empty program") {
    auto ast = parse_string("");
    auto *program = dynamic_cast<ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    CHECK(program->get_program().size() == 0);
  }

  // --- Error cases ----------------------------------------------------------

  TEST_CASE("unexpected token throws") {
    CHECK_THROWS_AS(parse_string(")"), core::CompilerException);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace blip
//****************************************************************************
