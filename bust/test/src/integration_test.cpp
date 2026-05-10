//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Integration tests — full programs loaded from
//*            bust/programs/, run end-to-end through the compiler
//*            pipeline, and asserted against header-comment manifests.
//*            See bust/testing.md for the testing strategy and feature
//*            matrix this suite is meant to cover.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <test/inc/integration_helpers.hpp>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.integration") {

  // --- Smoke / canonical examples ------------------------------------------

  TEST_CASE("hello_world") { RUN_INTEGRATION_PROGRAM("hello_world.bu"); }

  TEST_CASE("fibonacci") { RUN_INTEGRATION_PROGRAM("fibonacci.bu"); }

  // --- Lambdas / closures --------------------------------------------------

  TEST_CASE("lambda — higher-order apply") {
    RUN_INTEGRATION_PROGRAM("lambda.bu");
  }

  TEST_CASE("side_effect_return — naked return inside lambda") {
    RUN_INTEGRATION_PROGRAM("side_effect_return.bu");
  }

  // --- Polymorphism & control flow -----------------------------------------

  TEST_CASE("polymorphic — id lambda used at bool and i64") {
    RUN_INTEGRATION_PROGRAM("polymorphic.bu");
  }

  TEST_CASE("short_circuit — &&/|| chains") {
    RUN_INTEGRATION_PROGRAM("short_circuit.bu");
  }

  // --- Loop combinations ---------------------------------------------------

  TEST_CASE("loop_helper_lambda — closure called inside while") {
    RUN_INTEGRATION_PROGRAM("loop_helper_lambda.bu");
  }

  TEST_CASE("loop_with_recursion — recursive helper called in while body") {
    RUN_INTEGRATION_PROGRAM("loop_with_recursion.bu");
  }

  TEST_CASE("nested_loops_break — inner break only exits innermost loop") {
    RUN_INTEGRATION_PROGRAM("nested_loops_break.bu");
  }

  TEST_CASE("lambda_with_loop — lambda body containing while + break") {
    RUN_INTEGRATION_PROGRAM("lambda_with_loop.bu");
  }

  TEST_CASE("lambda_return_in_loop — lambda's return is scoped to lambda") {
    RUN_INTEGRATION_PROGRAM("lambda_return_in_loop.bu");
  }

  TEST_CASE("tuple_loop — tuple reassignment across iterations") {
    RUN_INTEGRATION_PROGRAM("tuple_loop.bu");
  }

  TEST_CASE("poly_in_loop — polymorphic id at i64 and bool") {
    RUN_INTEGRATION_PROGRAM("poly_in_loop.bu");
  }

  TEST_CASE("cast_chain_loop — casts in loop condition + body") {
    RUN_INTEGRATION_PROGRAM("cast_chain_loop.bu");
  }

  TEST_CASE("short_circuit_in_loop — &&/|| gating a break") {
    RUN_INTEGRATION_PROGRAM("short_circuit_in_loop.bu");
  }

  // --- loop / break-with-value / continue ----------------------------------

  TEST_CASE("loop_break_value — typed loop result via break <expr>") {
    RUN_INTEGRATION_PROGRAM("loop_break_value.bu");
  }

  TEST_CASE("continue_skip_evens — continue dispatches to while condition") {
    RUN_INTEGRATION_PROGRAM("continue_skip_evens.bu");
  }

  TEST_CASE("loop_break_in_lambda — lambda's own loop_stack") {
    RUN_INTEGRATION_PROGRAM("loop_break_in_lambda.bu");
  }

  TEST_CASE("loop_break_tuple — break carrying a tuple value") {
    RUN_INTEGRATION_PROGRAM("loop_break_tuple.bu");
  }

  // --- Catch-all -----------------------------------------------------------

  TEST_CASE("everything — single program touching most features") {
    RUN_INTEGRATION_PROGRAM("everything.bu");
  }

  // --- Negative tests (typecheck must reject) ------------------------------

  TEST_CASE("bad_poly — bool fails Numeric constraint") {
    RUN_INTEGRATION_PROGRAM("bad_poly.bu");
  }

  TEST_CASE("bad_poly_2 — mixed (i64, bool) fails unification") {
    RUN_INTEGRATION_PROGRAM("bad_poly_2.bu");
  }

  TEST_CASE("bad_continue_outside_loop — continue must be inside a loop") {
    RUN_INTEGRATION_PROGRAM("bad_continue_outside_loop.bu");
  }

  TEST_CASE("bad_break_value_in_while — while break must be unit-typed") {
    RUN_INTEGRATION_PROGRAM("bad_break_value_in_while.bu");
  }
}

//****************************************************************************
} // namespace bust
//****************************************************************************
