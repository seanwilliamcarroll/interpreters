//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for blip::TypeChecker
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <lexer.hpp>
#include <parser.hpp>
#include <type.hpp>
#include <type_checker.hpp>

#include <doctest/doctest.h>
#include <sstream>

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.type_checker") {

  // Helper: parse and type-check a program, return the result type
  static Type check(const std::string &source,
                    std::shared_ptr<TypeEnvironment> env = nullptr) {
    if (!env) {
      env = std::make_shared<TypeEnvironment>();
    }
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    auto ast = parser.parse();
    TypeChecker checker(env);
    return checker.check(*ast);
  }

  static Type check_with_builtins(const std::string &source) {
    auto env = default_type_environment();
    return check(source, env);
  }

  // --- Literals ------------------------------------------------------------

  TEST_CASE("int literal has type Int") { CHECK(check("42") == Type::Int); }

  TEST_CASE("double literal has type Double") {
    CHECK(check("3.14") == Type::Double);
  }

  TEST_CASE("string literal has type String") {
    CHECK(check("\"hello\"") == Type::String);
  }

  TEST_CASE("bool literal has type Bool") {
    CHECK(check("true") == Type::Bool);
    CHECK(check("false") == Type::Bool);
  }

  // --- Program and begin ---------------------------------------------------

  TEST_CASE("program result is last expression type") {
    CHECK(check("1 \"hello\" true") == Type::Bool);
  }

  TEST_CASE("begin result is last expression type") {
    CHECK(check("(begin 1 2 true)") == Type::Bool);
  }

  // --- Define variable (inferred) ------------------------------------------

  TEST_CASE("define variable infers type from initializer") {
    CHECK(check("(begin (define x 5) x)") == Type::Int);
  }

  TEST_CASE("define variable infers string type") {
    CHECK(check("(begin (define x \"hi\") x)") == Type::String);
  }

  TEST_CASE("define variable result is unit") {
    CHECK(check("(define x 5)") == Type::Unit);
  }

  // --- Define variable (annotated) -----------------------------------------

  TEST_CASE("define variable with matching annotation") {
    CHECK(check("(begin (define x : int 5) x)") == Type::Int);
  }

  TEST_CASE("define variable with wrong annotation throws") {
    CHECK_THROWS(check("(define x : bool 5)"));
  }

  // --- Identifier ----------------------------------------------------------

  TEST_CASE("identifier looks up type") {
    auto env = std::make_shared<TypeEnvironment>();
    env->define("x", Type::Int);
    CHECK(check("x", env) == Type::Int);
  }

  TEST_CASE("undefined identifier throws") { CHECK_THROWS(check("x")); }

  // --- Set -----------------------------------------------------------------

  TEST_CASE("set with same type is ok") {
    CHECK(check("(begin (define x 5) (set x 10))") == Type::Unit);
  }

  TEST_CASE("set with different type throws") {
    CHECK_THROWS(check("(begin (define x 5) (set x true))"));
  }

  TEST_CASE("set result is unit") {
    CHECK(check("(begin (define x 5) (set x 10))") == Type::Unit);
  }

  // --- Print ---------------------------------------------------------------

  TEST_CASE("print accepts any type") {
    CHECK(check("(print 42)") == Type::Unit);
    CHECK(check("(print \"hello\")") == Type::Unit);
    CHECK(check("(print true)") == Type::Unit);
  }

  // --- If ------------------------------------------------------------------

  TEST_CASE("if condition must be bool") { CHECK_THROWS(check("(if 1 2 3)")); }

  TEST_CASE("if branches must have same type") {
    CHECK(check("(if true 1 2)") == Type::Int);
  }

  TEST_CASE("if branches with mismatched types throws") {
    CHECK_THROWS(check("(if true 1 \"hello\")"));
  }

  TEST_CASE("if without else requires unit then-branch") {
    CHECK(check("(if true (print 1))") == Type::Unit);
  }

  TEST_CASE("if without else with non-unit then-branch throws") {
    CHECK_THROWS(check("(if true 1)"));
  }

  // --- While ---------------------------------------------------------------

  TEST_CASE("while condition must be bool") {
    CHECK_THROWS(check("(while 1 2)"));
  }

  TEST_CASE("while result is unit") {
    CHECK(check("(while false 1)") == Type::Unit);
  }

  // --- Define function -----------------------------------------------------

  TEST_CASE("define function result is unit") {
    CHECK(check("(define (f) : int 42)") == Type::Unit);
  }

  TEST_CASE("define function binds name as Fn") {
    auto env = std::make_shared<TypeEnvironment>();
    check("(define (f) : int 42)", env);
    CHECK(env->lookup("f") == Type::Fn);
  }

  TEST_CASE("function body must match return type") {
    CHECK_NOTHROW(check("(define (f) : int 42)"));
  }

  TEST_CASE("function body wrong return type throws") {
    CHECK_THROWS(check("(define (f) : int true)"));
  }

  TEST_CASE("function params have declared types in body") {
    CHECK_NOTHROW(check("(define (f (x : int)) : int x)"));
  }

  TEST_CASE("function param type mismatch in body throws") {
    // Body returns a string but declared return is int
    CHECK_THROWS(check("(define (f (x : string)) : int x)"));
  }

  // --- Function calls (opaque Fn) ------------------------------------------

  TEST_CASE("calling Fn-typed value does not throw") {
    // Fn is opaque — no argument or return type checking
    CHECK_NOTHROW(check("(begin (define (f) : int 42) (f))"));
  }

  TEST_CASE("calling non-function throws") {
    CHECK_THROWS(check("(begin (define x 5) (x))"));
  }

  // --- Built-ins (opaque Fn) -----------------------------------------------

  TEST_CASE("built-in functions are Fn typed") {
    auto env = default_type_environment();
    CHECK(env->lookup("+") == Type::Fn);
    CHECK(env->lookup("-") == Type::Fn);
    CHECK(env->lookup("*") == Type::Fn);
    CHECK(env->lookup("/") == Type::Fn);
    CHECK(env->lookup(">") == Type::Fn);
    CHECK(env->lookup("<") == Type::Fn);
    CHECK(env->lookup("=") == Type::Fn);
  }

  TEST_CASE("calling built-in does not throw") {
    CHECK_NOTHROW(check_with_builtins("(+ 1 2)"));
  }

  // --- Integration ---------------------------------------------------------

  TEST_CASE("well-typed program with function and builtins") {
    CHECK_NOTHROW(
        check_with_builtins("(begin"
                            "  (define (add-one (x : int)) : int (+ x 1))"
                            "  (add-one 5))"));
  }

  TEST_CASE("well-typed factorial") {
    CHECK_NOTHROW(check_with_builtins("(begin"
                                      "  (define (factorial (n : int)) : int"
                                      "    (if (= n 0)"
                                      "      1"
                                      "      (* n (factorial (- n 1)))))"
                                      "  (factorial 5))"));
  }

  TEST_CASE("well-typed closure") {
    CHECK_NOTHROW(check_with_builtins("(begin"
                                      "  (define x 10)"
                                      "  (define (get-x) : int x)"
                                      "  (get-x))"));
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
