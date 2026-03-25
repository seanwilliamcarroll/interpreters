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

#include <parser.hpp>          // For Parser

#include <blip.hpp>            // For blip token types
#include <ast.hpp>             // For blip AST nodes
#include <sc/ast.hpp>          // For core AST nodes
#include <sc/sc.hpp>           // For make_lexer
#include <sc/token.hpp>        // For Token

#include <doctest/doctest.h>   // For doctest
#include <sstream>             // For istringstream

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.parser") {

// Helper: parse a string and return the AST root
static std::unique_ptr<sc::AstNode> parse_string(const std::string &source) {
  std::istringstream input(source);
  auto lexer = sc::make_lexer(input, {
    {"if",     Blip::IF},
    {"while",  Blip::WHILE},
    {"set",    Blip::SET},
    {"begin",  Blip::BEGIN},
    {"print",  Blip::PRINT},
    {"define", Blip::DEFINE}
  }, "test");
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

  auto *lit = dynamic_cast<sc::StringLiteral *>(program->get_program()[0].get());
  REQUIRE(lit != nullptr);
  CHECK(lit->get_value() == "hello");
}

TEST_CASE("parse bool literal") {
  auto ast = parse_string("true");
  auto *program = dynamic_cast<sc::ProgramNode *>(ast.get());
  REQUIRE(program != nullptr);
  REQUIRE(program->get_program().size() == 1);

  auto *lit = dynamic_cast<sc::BoolLiteral *>(program->get_program()[0].get());
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
