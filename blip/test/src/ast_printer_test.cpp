//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Round-trip tests for blip::AstPrinter
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <ast_printer.hpp> // For AstPrinter
#include <parser.hpp>      // For Parser

#include <blip.hpp>   // For blip token types
#include <sc/ast.hpp> // For core AST nodes
#include <sc/sc.hpp>  // For make_lexer

#include <doctest/doctest.h> // For doctest
#include <sstream>           // For istringstream

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.ast_printer") {

  // Helper: parse a string into an AST
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

  // Helper: parse then print
  static std::string print_source(const std::string &source) {
    auto ast = parse_string(source);
    AstPrinter printer;
    return printer.print(*ast);
  }

  // Round-trip helper: parse → print → parse → print, check both prints match
  static void check_round_trip(const std::string &source) {
    std::string first_print = print_source(source);
    std::string second_print = print_source(first_print);
    CHECK(first_print == second_print);
  }

  // --- Atoms ----------------------------------------------------------------

  TEST_CASE("round-trip integer literal") { check_round_trip("42"); }

  TEST_CASE("round-trip negative integer literal") { check_round_trip("-7"); }

  TEST_CASE("round-trip zero") { check_round_trip("0"); }

  TEST_CASE("round-trip double literal") { check_round_trip("3.14"); }

  TEST_CASE("round-trip string literal") { check_round_trip("\"hello\""); }

  TEST_CASE("round-trip empty string literal") { check_round_trip("\"\""); }

  TEST_CASE("round-trip string with spaces") {
    check_round_trip("\"hello world\"");
  }

  TEST_CASE("round-trip bool true") { check_round_trip("true"); }

  TEST_CASE("round-trip bool false") { check_round_trip("false"); }

  TEST_CASE("round-trip identifier") { check_round_trip("x"); }

  TEST_CASE("round-trip multi-char identifier") { check_round_trip("foobar"); }

  // --- Multiple top-level expressions ---------------------------------------

  TEST_CASE("round-trip multiple atoms") { check_round_trip("1 2 3"); }

  TEST_CASE("round-trip mixed atoms") {
    check_round_trip("42 \"hello\" true x");
  }

  // --- Function calls -------------------------------------------------------

  TEST_CASE("round-trip function call no args") { check_round_trip("(f)"); }

  TEST_CASE("round-trip function call one arg") { check_round_trip("(f 1)"); }

  TEST_CASE("round-trip function call multiple args") {
    check_round_trip("(f 1 2 3)");
  }

  TEST_CASE("round-trip nested function calls") {
    check_round_trip("(f (g 1) (h 2 3))");
  }

  TEST_CASE("round-trip deeply nested calls") {
    check_round_trip("(f (g (h (i 1))))");
  }

  // --- Special forms: if ----------------------------------------------------

  TEST_CASE("round-trip if without else") { check_round_trip("(if true 1)"); }

  TEST_CASE("round-trip if with else") { check_round_trip("(if true 1 2)"); }

  TEST_CASE("round-trip if with complex condition") {
    check_round_trip("(if (eq x 0) 1 2)");
  }

  TEST_CASE("round-trip nested if") {
    check_round_trip("(if true (if false 1 2) 3)");
  }

  // --- Special forms: while -------------------------------------------------

  TEST_CASE("round-trip while") { check_round_trip("(while true 1)"); }

  TEST_CASE("round-trip while with complex body") {
    check_round_trip("(while (lt x 10) (set x (add x 1)))");
  }

  // --- Special forms: set ---------------------------------------------------

  TEST_CASE("round-trip set") { check_round_trip("(set x 42)"); }

  TEST_CASE("round-trip set with expression value") {
    check_round_trip("(set x (add 1 2))");
  }

  // --- Special forms: begin -------------------------------------------------

  TEST_CASE("round-trip begin single expression") {
    check_round_trip("(begin 1)");
  }

  TEST_CASE("round-trip begin multiple expressions") {
    check_round_trip("(begin 1 2 3)");
  }

  TEST_CASE("round-trip begin with nested forms") {
    check_round_trip("(begin (set x 1) (set y 2) (add x y))");
  }

  // --- Special forms: print -------------------------------------------------

  TEST_CASE("round-trip print") { check_round_trip("(print 42)"); }

  TEST_CASE("round-trip print expression") {
    check_round_trip("(print (add 1 2))");
  }

  // --- Special forms: define variable ---------------------------------------

  TEST_CASE("round-trip define variable") { check_round_trip("(define x 42)"); }

  TEST_CASE("round-trip define variable with expression") {
    check_round_trip("(define x (add 1 2))");
  }

  // --- Special forms: define function ---------------------------------------

  TEST_CASE("round-trip define function no params") {
    check_round_trip("(define (f) 42)");
  }

  TEST_CASE("round-trip define function one param") {
    check_round_trip("(define (f x) x)");
  }

  TEST_CASE("round-trip define function multiple params") {
    check_round_trip("(define (f x y z) (add x (add y z)))");
  }

  // --- Compound programs ----------------------------------------------------

  TEST_CASE("round-trip define then call") {
    check_round_trip("(define (square x) (mul x x)) (square 5)");
  }

  TEST_CASE("round-trip complex program") {
    check_round_trip("(define (factorial n) "
                     "  (if (eq n 0) "
                     "    1 "
                     "    (mul n (factorial (sub n 1)))))");
  }

  TEST_CASE("round-trip program with multiple defines and calls") {
    check_round_trip("(define x 10) "
                     "(define (double n) (add n n)) "
                     "(print (double x))");
  }

  TEST_CASE("round-trip while loop program") {
    check_round_trip("(define x 0) "
                     "(while (lt x 10) "
                     "  (begin "
                     "    (print x) "
                     "    (set x (add x 1))))");
  }

  // --- Empty program --------------------------------------------------------

  TEST_CASE("round-trip empty program") { check_round_trip(""); }

} // TEST_SUITE
//****************************************************************************
} // namespace blip
//****************************************************************************
