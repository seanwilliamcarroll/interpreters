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

  TEST_CASE("int literal has type Int") { CHECK(check("42") == BaseType::Int); }

  TEST_CASE("double literal has type Double") {
    CHECK(check("3.14") == BaseType::Double);
  }

  TEST_CASE("string literal has type String") {
    CHECK(check("\"hello\"") == BaseType::String);
  }

  TEST_CASE("bool literal has type Bool") {
    CHECK(check("true") == BaseType::Bool);
    CHECK(check("false") == BaseType::Bool);
  }

  // --- Program and begin ---------------------------------------------------

  TEST_CASE("program result is last expression type") {
    CHECK(check("1 \"hello\" true") == BaseType::Bool);
  }

  TEST_CASE("begin result is last expression type") {
    CHECK(check("(begin 1 2 true)") == BaseType::Bool);
  }

  // --- Define variable (inferred) ------------------------------------------

  TEST_CASE("define variable infers type from initializer") {
    CHECK(check("(begin (define x 5) x)") == BaseType::Int);
  }

  TEST_CASE("define variable infers string type") {
    CHECK(check("(begin (define x \"hi\") x)") == BaseType::String);
  }

  TEST_CASE("define variable result is unit") {
    CHECK(check("(define x 5)") == BaseType::Unit);
  }

  // --- Define variable (annotated) -----------------------------------------

  TEST_CASE("define variable with matching annotation") {
    CHECK(check("(begin (define x : int 5) x)") == BaseType::Int);
  }

  TEST_CASE("define variable with wrong annotation throws") {
    CHECK_THROWS(check("(define x : bool 5)"));
  }

  // --- Identifier ----------------------------------------------------------

  TEST_CASE("identifier looks up type") {
    auto env = std::make_shared<TypeEnvironment>();
    env->define("x", BaseType::Int);
    CHECK(check("x", env) == BaseType::Int);
  }

  TEST_CASE("undefined identifier throws") { CHECK_THROWS(check("x")); }

  // --- Set -----------------------------------------------------------------

  TEST_CASE("set with same type is ok") {
    CHECK(check("(begin (define x 5) (set x 10))") == BaseType::Unit);
  }

  TEST_CASE("set with different type throws") {
    CHECK_THROWS(check("(begin (define x 5) (set x true))"));
  }

  TEST_CASE("set result is unit") {
    CHECK(check("(begin (define x 5) (set x 10))") == BaseType::Unit);
  }

  // --- Print ---------------------------------------------------------------

  TEST_CASE("print accepts any type") {
    CHECK(check("(print 42)") == BaseType::Unit);
    CHECK(check("(print \"hello\")") == BaseType::Unit);
    CHECK(check("(print true)") == BaseType::Unit);
  }

  // --- If ------------------------------------------------------------------

  TEST_CASE("if condition must be bool") { CHECK_THROWS(check("(if 1 2 3)")); }

  TEST_CASE("if branches must have same type") {
    CHECK(check("(if true 1 2)") == BaseType::Int);
  }

  TEST_CASE("if branches with mismatched types throws") {
    CHECK_THROWS(check("(if true 1 \"hello\")"));
  }

  TEST_CASE("if without else requires unit then-branch") {
    CHECK(check("(if true (print 1))") == BaseType::Unit);
  }

  TEST_CASE("if without else with non-unit then-branch throws") {
    CHECK_THROWS(check("(if true 1)"));
  }

  // --- While ---------------------------------------------------------------

  TEST_CASE("while condition must be bool") {
    CHECK_THROWS(check("(while 1 2)"));
  }

  TEST_CASE("while result is unit") {
    CHECK(check("(while false 1)") == BaseType::Unit);
  }

  // --- Define function -----------------------------------------------------

  TEST_CASE("define function result is unit") {
    CHECK(check("(define (f) : int 42)") == BaseType::Unit);
  }

  TEST_CASE("define function binds name as function type") {
    auto env = std::make_shared<TypeEnvironment>();
    check("(define (f) : int 42)", env);
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup("f")));
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

  // --- Function calls -------------------------------------------------------

  TEST_CASE("call returns declared return type") {
    CHECK(check("(begin (define (f) : int 42) (f))") == BaseType::Int);
  }

  TEST_CASE("call with correct arg types does not throw") {
    CHECK_NOTHROW(check("(begin (define (f (x : int)) : int x) (f 5))"));
  }

  TEST_CASE("call with wrong arg type throws") {
    CHECK_THROWS(check("(begin (define (f (x : int)) : int x) (f true))"));
  }

  TEST_CASE("call with wrong arity throws") {
    CHECK_THROWS(check("(begin (define (f (x : int)) : int x) (f 1 2))"));
    CHECK_THROWS(check("(begin (define (f (x : int)) : int x) (f))"));
  }

  TEST_CASE("calling non-function throws") {
    CHECK_THROWS(check("(begin (define x 5) (x))"));
  }

  // --- Built-ins -------------------------------------------------------------

  TEST_CASE("built-in functions are function typed") {
    auto env = default_type_environment();
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup("+")));
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup("-")));
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup("*")));
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup("/")));
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup(">")));
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup("<")));
    CHECK(std::holds_alternative<std::shared_ptr<FunctionType>>(
        env->lookup("=")));
  }

  TEST_CASE("calling built-in returns correct type") {
    CHECK(check_with_builtins("(+ 1 2)") == BaseType::Int);
  }

  TEST_CASE("calling built-in with wrong arg types throws") {
    CHECK_THROWS(check_with_builtins("(+ true 2)"));
    CHECK_THROWS(check_with_builtins("(+ 1 \"hello\")"));
  }

  TEST_CASE("comparison built-in returns bool") {
    CHECK(check_with_builtins("(> 1 2)") == BaseType::Bool);
    CHECK(check_with_builtins("(< 1 2)") == BaseType::Bool);
    CHECK(check_with_builtins("(= 1 2)") == BaseType::Bool);
  }

  // --- Function type annotations -------------------------------------------

  TEST_CASE("function with function-typed param accepts matching function") {
    // apply takes (int -> int) and an int, calls f on x
    CHECK_NOTHROW(check_with_builtins(
        "(begin"
        "  (define (inc (x : int)) : int (+ x 1))"
        "  (define (apply (f : (int -> int)) (x : int)) : int (f x))"
        "  (apply inc 5))"));
  }

  TEST_CASE("function with function-typed param returns correct type") {
    CHECK(check_with_builtins(
              "(begin"
              "  (define (inc (x : int)) : int (+ x 1))"
              "  (define (apply (f : (int -> int)) (x : int)) : int (f x))"
              "  (apply inc 5))") == BaseType::Int);
  }

  TEST_CASE("passing wrong function type throws") {
    // inc is (int -> int), but apply expects (int -> bool)
    CHECK_THROWS(check_with_builtins(
        "(begin"
        "  (define (inc (x : int)) : int (+ x 1))"
        "  (define (apply (f : (int -> bool)) (x : int)) : bool (f x))"
        "  (apply inc 5))"));
  }

  TEST_CASE("passing non-function where function type expected throws") {
    CHECK_THROWS(check_with_builtins(
        "(begin"
        "  (define (apply (f : (int -> int)) (x : int)) : int (f x))"
        "  (apply 42 5))"));
  }

  TEST_CASE("passing function with wrong arity throws") {
    // add takes two ints, but apply expects (int -> int) — one param
    CHECK_THROWS(check_with_builtins(
        "(begin"
        "  (define (add (a : int) (b : int)) : int (+ a b))"
        "  (define (apply (f : (int -> int)) (x : int)) : int (f x))"
        "  (apply add 5))"));
  }

  TEST_CASE("function returning function type") {
    // make-inc returns a function, we can call the result
    CHECK(check_with_builtins("(begin"
                              "  (define (inc (x : int)) : int (+ x 1))"
                              "  (define (make-inc) : (int -> int) inc)"
                              "  ((make-inc) 5))") == BaseType::Int);
  }

  TEST_CASE(
      "function returning wrong type for function return annotation throws") {
    // Declares return type (int -> int) but body returns an int
    CHECK_THROWS(check_with_builtins("(begin"
                                     "  (define (bad) : (int -> int) 42))"));
  }

  TEST_CASE("apply-twice higher-order pattern") {
    CHECK(check_with_builtins(
              "(begin"
              "  (define (inc (x : int)) : int (+ x 1))"
              "  (define (apply-twice (f : (int -> int)) (x : int)) : int"
              "    (f (f x)))"
              "  (apply-twice inc 5))") == BaseType::Int);
  }

  TEST_CASE("multi-param function type annotation") {
    // f takes a two-argument function (int int -> int)
    CHECK(
        check_with_builtins("(begin"
                            "  (define (add (a : int) (b : int)) : int (+ a b))"
                            "  (define (apply-binop (f : (int int -> int))"
                            "                       (a : int) (b : int)) : int"
                            "    (f a b))"
                            "  (apply-binop add 3 4))") == BaseType::Int);
  }

  TEST_CASE("nested function type in parameter: ((int -> int) -> int)") {
    CHECK_NOTHROW(check_with_builtins(
        "(begin"
        "  (define (inc (x : int)) : int (+ x 1))"
        "  (define (apply-to-zero (f : (int -> int))) : int (f 0))"
        "  (define (meta (g : ((int -> int) -> int))) : int (g inc))"
        "  (meta apply-to-zero))"));
  }

  TEST_CASE("function with return type (int -> (int -> int))") {
    // Return type is itself a function that returns a function
    CHECK_NOTHROW(
        check_with_builtins("(begin"
                            "  (define (inc (x : int)) : int (+ x 1))"
                            "  (define (wrap) : (int -> int) inc)"
                            "  (define (get-wrap) : (-> (int -> int)) wrap)"
                            "))"));
  }

  TEST_CASE("calling result of function that returns function") {
    CHECK(check_with_builtins("(begin"
                              "  (define (inc (x : int)) : int (+ x 1))"
                              "  (define (get-inc) : (int -> int) inc)"
                              "  ((get-inc) 10))") == BaseType::Int);
  }

  TEST_CASE(
      "function type mismatch in body — calls param with wrong arg type") {
    // f is (int -> int) but we call it with a bool
    CHECK_THROWS(check_with_builtins("(begin"
                                     "  (define (bad (f : (int -> int))) : int"
                                     "    (f true)))"));
  }

  TEST_CASE("function type annotation on param allows calling param") {
    // Type checker must know f is callable with the right types
    CHECK_NOTHROW(check_with_builtins(
        "(begin"
        "  (define (apply (f : (int -> int)) (x : int)) : int"
        "    (f x)))"));
  }

  TEST_CASE("define variable with function type annotation") {
    // Define a variable whose type annotation is a function type
    CHECK_NOTHROW(check_with_builtins("(begin"
                                      "  (define (inc (x : int)) : int (+ x 1))"
                                      "  (define f : (int -> int) inc)"
                                      "  (f 5))"));
  }

  // --- Integration ---------------------------------------------------------

  TEST_CASE("well-typed program with function and builtins") {
    CHECK(check_with_builtins("(begin"
                              "  (define (add-one (x : int)) : int (+ x 1))"
                              "  (add-one 5))") == BaseType::Int);
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
