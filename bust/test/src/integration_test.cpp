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

  // --- Negative tests (typecheck must reject) ------------------------------

  TEST_CASE("bad_poly — bool fails Numeric constraint") {
    RUN_INTEGRATION_PROGRAM("bad_poly.bu");
  }

  TEST_CASE("bad_poly_2 — mixed (i64, bool) fails unification") {
    RUN_INTEGRATION_PROGRAM("bad_poly_2.bu");
  }
}

//****************************************************************************
} // namespace bust
//****************************************************************************
