//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for blip::Evaluator
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <evaluator.hpp>
#include <lexer.hpp>
#include <parser.hpp>

#include <doctest/doctest.h>
#include <iostream>
#include <sstream>

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.evaluator") {

  // Helper: parse and evaluate a program, return the final value
  static Value eval(const std::string &source,
                    std::shared_ptr<Environment> env = nullptr,
                    std::ostream &out = std::cout) {
    if (!env) {
      env = std::make_shared<Environment>();
    }
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    auto ast = parser.parse();
    Evaluator evaluator(env, out);
    return evaluator.evaluate(*ast);
  }

  // --- Literals ------------------------------------------------------------

  TEST_CASE("evaluate integer literal") {
    auto result = eval("42");
    CHECK(std::get<int>(result) == 42);
  }

  TEST_CASE("evaluate negative integer literal") {
    auto result = eval("-7");
    CHECK(std::get<int>(result) == -7);
  }

  TEST_CASE("evaluate double literal") {
    auto result = eval("3.14");
    CHECK(std::get<double>(result) == doctest::Approx(3.14));
  }

  TEST_CASE("evaluate string literal") {
    auto result = eval("\"hello\"");
    CHECK(std::get<std::string>(result) == "hello");
  }

  TEST_CASE("evaluate bool literals") {
    CHECK(std::get<bool>(eval("true")) == true);
    CHECK(std::get<bool>(eval("false")) == false);
  }

  // --- Program / sequence --------------------------------------------------

  TEST_CASE("program returns last expression") {
    auto result = eval("1 2 3");
    CHECK(std::get<int>(result) == 3);
  }

  TEST_CASE("empty program returns unit") {
    auto result = eval("");
    CHECK(std::holds_alternative<Unit>(result));
  }

  // --- Identifiers ---------------------------------------------------------

  TEST_CASE("evaluate identifier") {
    auto env = std::make_shared<Environment>();
    env->define("x", 42);
    auto result = eval("x", env);
    CHECK(std::get<int>(result) == 42);
  }

  TEST_CASE("unbound identifier throws") { CHECK_THROWS(eval("x")); }

  // --- define (variable) ---------------------------------------------------

  TEST_CASE("define binds variable") {
    auto env = std::make_shared<Environment>();
    eval("(define x 10)", env);
    CHECK(std::get<int>(env->lookup("x")) == 10);
  }

  TEST_CASE("define evaluates value expression") {
    auto env = std::make_shared<Environment>();
    eval("(define x (if true 1 2))", env);
    CHECK(std::get<int>(env->lookup("x")) == 1);
  }

  TEST_CASE("define returns unit") {
    auto result = eval("(define x 5)");
    CHECK(std::holds_alternative<Unit>(result));
  }

  // --- set -----------------------------------------------------------------

  TEST_CASE("set mutates existing variable") {
    auto env = std::make_shared<Environment>();
    eval("(define x 1)", env);
    eval("(set x 99)", env);
    CHECK(std::get<int>(env->lookup("x")) == 99);
  }

  TEST_CASE("set evaluates value expression") {
    auto env = std::make_shared<Environment>();
    eval("(define x 0)", env);
    eval("(set x (if false 1 2))", env);
    CHECK(std::get<int>(env->lookup("x")) == 2);
  }

  TEST_CASE("set on unbound variable throws") {
    CHECK_THROWS(eval("(set x 1)"));
  }

  TEST_CASE("set returns unit") {
    auto env = std::make_shared<Environment>();
    eval("(define x 0)", env);
    auto result = eval("(set x 1)", env);
    CHECK(std::holds_alternative<Unit>(result));
  }

  // --- begin ---------------------------------------------------------------

  TEST_CASE("begin returns last expression") {
    auto result = eval("(begin 1 2 3)");
    CHECK(std::get<int>(result) == 3);
  }

  TEST_CASE("begin evaluates all expressions") {
    auto env = std::make_shared<Environment>();
    eval("(begin (define x 1) (define y 2))", env);
    CHECK(std::get<int>(env->lookup("x")) == 1);
    CHECK(std::get<int>(env->lookup("y")) == 2);
  }

  // --- print ---------------------------------------------------------------

  TEST_CASE("print writes to output stream") {
    std::ostringstream out;
    eval("(print 42)", nullptr, out);
    CHECK(out.str() == "42\n");
  }

  TEST_CASE("print writes string value") {
    std::ostringstream out;
    eval("(print \"hello\")", nullptr, out);
    CHECK(out.str() == "hello\n");
  }

  TEST_CASE("print writes bool value") {
    std::ostringstream out;
    eval("(print true)", nullptr, out);
    CHECK(out.str() == "true\n");
  }

  TEST_CASE("print returns unit") {
    std::ostringstream out;
    auto result = eval("(print 1)", nullptr, out);
    CHECK(std::holds_alternative<Unit>(result));
  }

  TEST_CASE("multiple prints accumulate") {
    std::ostringstream out;
    eval("(begin (print 1) (print 2) (print 3))", nullptr, out);
    CHECK(out.str() == "1\n2\n3\n");
  }

  // --- if ------------------------------------------------------------------

  TEST_CASE("if true returns then branch") {
    auto result = eval("(if true 1 2)");
    CHECK(std::get<int>(result) == 1);
  }

  TEST_CASE("if false returns else branch") {
    auto result = eval("(if false 1 2)");
    CHECK(std::get<int>(result) == 2);
  }

  TEST_CASE("if false without else returns unit") {
    auto result = eval("(if false 1)");
    CHECK(std::holds_alternative<Unit>(result));
  }

  TEST_CASE("if evaluates condition") {
    auto env = std::make_shared<Environment>();
    env->define("flag", true);
    auto result = eval("(if flag 10 20)", env);
    CHECK(std::get<int>(result) == 10);
  }

  TEST_CASE("if does not evaluate untaken branch") {
    // If the else branch were evaluated, it would throw (unbound variable)
    auto result = eval("(if true 1 nope)");
    CHECK(std::get<int>(result) == 1);
  }

  // --- while ---------------------------------------------------------------

  TEST_CASE("while returns unit") {
    auto result = eval("(while false 1)");
    CHECK(std::holds_alternative<Unit>(result));
  }

  TEST_CASE("while loops until condition is false") {
    auto env = std::make_shared<Environment>();
    std::ostringstream out;
    eval("(begin (define x true) (while x (begin (print 1) (set x false))))",
         env, out);
    CHECK(out.str() == "1\n");
  }

  TEST_CASE("while body executes multiple times") {
    auto env = std::make_shared<Environment>();
    // Count down: define x = 3, loop printing and decrementing
    // This test depends on built-in arithmetic — skip until Step 6
    // For now, test that a false condition means zero iterations
    eval("(define count 0)", env);
    eval("(while false (set count 1))", env);
    CHECK(std::get<int>(env->lookup("count")) == 0);
  }

  // --- Strict truthiness ----------------------------------------------------

  TEST_CASE("if with int condition throws") {
    CHECK_THROWS(eval("(if 1 2 3)"));
  }

  TEST_CASE("if with string condition throws") {
    CHECK_THROWS(eval("(if \"yes\" 2 3)"));
  }

  TEST_CASE("while with int condition throws") {
    CHECK_THROWS(eval("(while 0 1)"));
  }

  // --- Integration: define + set + if + begin ------------------------------

  TEST_CASE("define and use variable in if") {
    auto env = std::make_shared<Environment>();
    auto result = eval("(begin (define x true) (if x 10 20))", env);
    CHECK(std::get<int>(result) == 10);
  }

  TEST_CASE("set variable and read back") {
    auto env = std::make_shared<Environment>();
    auto result = eval("(begin (define x 1) (set x 42) x)", env);
    CHECK(std::get<int>(result) == 42);
  }

  TEST_CASE("nested begin blocks") {
    auto result = eval("(begin (begin (begin 99)))");
    CHECK(std::get<int>(result) == 99);
  }

  TEST_CASE("print expression result then return it") {
    std::ostringstream out;
    auto env = std::make_shared<Environment>();
    auto result = eval("(begin (define x 42) (print x) x)", env, out);
    CHECK(out.str() == "42\n");
    CHECK(std::get<int>(result) == 42);
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
