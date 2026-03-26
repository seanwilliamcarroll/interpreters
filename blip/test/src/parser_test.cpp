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

#include <parser.hpp> // For Parser

#include <ast.hpp>      // For blip AST nodes
#include <blip.hpp>     // For blip token types
#include <sc/ast.hpp>   // For core AST nodes
#include <sc/sc.hpp>    // For make_lexer
#include <sc/token.hpp> // For Token

#include <doctest/doctest.h> // For doctest
#include <sstream>           // For istringstream

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.parser") {

  // Helper: parse a string and return the AST root
  static std::unique_ptr<sc::AstNode> parse_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = sc::make_lexer(input,
                                {{"if", BlipToken::IF},
                                 {"while", BlipToken::WHILE},
                                 {"set", BlipToken::SET},
                                 {"begin", BlipToken::BEGIN},
                                 {"print", BlipToken::PRINT},
                                 {"define", BlipToken::DEFINE}},
                                "test");
    Parser parser(std::move(lexer));
    return parser.parse();
  }

  // --- Atoms ----------------------------------------------------------------

  TEST_CASE("parse integer literal") {
    auto ast = parse_string("42");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit = dynamic_cast<sc::IntLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == 42);
  }

  TEST_CASE("parse string literal") {
    auto ast = parse_string("\"hello\"");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit =
        dynamic_cast<sc::StringLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == "hello");
  }

  TEST_CASE("parse double literal") {
    auto ast = parse_string("34.9999");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit =
        dynamic_cast<sc::DoubleLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == 34.9999);
  }

  TEST_CASE("parse bool literal") {
    auto ast = parse_string("true");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *lit =
        dynamic_cast<sc::BoolLiteral *>(program->get_program()[0].get());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == true);
  }

  TEST_CASE("parse identifier") {
    auto ast = parse_string("foo");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *id = dynamic_cast<sc::Identifier *>(program->get_program()[0].get());
    REQUIRE(id != nullptr);
    CHECK(id->get_name() == "foo");
  }

  // --- Special forms --------------------------------------------------------

  TEST_CASE("parse nonsense program") {
    auto ast = parse_string("42\n54\ntrue\n34.9999\n\"blah blah blah\"");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 5);

    auto *int_0 =
        dynamic_cast<sc::IntLiteral *>(program->get_program()[0].get());
    REQUIRE(int_0 != nullptr);
    CHECK(int_0->get_value() == 42);

    auto *int_1 =
        dynamic_cast<sc::IntLiteral *>(program->get_program()[1].get());
    REQUIRE(int_1 != nullptr);
    CHECK(int_1->get_value() == 54);

    auto *bool_0 =
        dynamic_cast<sc::BoolLiteral *>(program->get_program()[2].get());
    REQUIRE(bool_0 != nullptr);
    CHECK(bool_0->get_value() == true);

    auto *double_0 =
        dynamic_cast<sc::DoubleLiteral *>(program->get_program()[3].get());
    REQUIRE(double_0 != nullptr);
    // Not sure this will work
    CHECK(double_0->get_value() == 34.9999);

    auto *string_0 =
        dynamic_cast<sc::StringLiteral *>(program->get_program()[4].get());
    REQUIRE(string_0 != nullptr);
    CHECK(string_0->get_value() == "blah blah blah");
  }

  TEST_CASE("parse print") {
    auto ast = parse_string("(print 42)");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    REQUIRE(program->get_program().size() == 1);

    auto *print = dynamic_cast<PrintNode *>(program->get_program()[0].get());
    REQUIRE(print != nullptr);

    auto *lit = dynamic_cast<const sc::IntLiteral *>(&print->get_expression());
    REQUIRE(lit != nullptr);
    CHECK(lit->get_value() == 42);
  }

  TEST_CASE("parse empty program") {
    auto ast = parse_string("");
    auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
    REQUIRE(program != nullptr);
    CHECK(program->get_program().size() == 0);
  }

  // --- Error cases ----------------------------------------------------------

  TEST_CASE("unexpected token throws") {
    CHECK_THROWS_AS(parse_string(")"), sc::ParserException);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace blip
//****************************************************************************
