//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for blip::Environment
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <environment.hpp>

#include <doctest/doctest.h>

//****************************************************************************
namespace blip {
//****************************************************************************
TEST_SUITE("blip.environment") {

  // --- Define and lookup ----------------------------------------------------

  TEST_CASE("define and lookup in single scope") {
    auto env = std::make_shared<Environment>();
    env->define("x", 42);
    CHECK(std::get<int>(env->lookup("x")) == 42);
  }

  TEST_CASE("define multiple bindings") {
    auto env = std::make_shared<Environment>();
    env->define("a", 1);
    env->define("b", std::string("hello"));
    env->define("c", true);
    CHECK(std::get<int>(env->lookup("a")) == 1);
    CHECK(std::get<std::string>(env->lookup("b")) == "hello");
    CHECK(std::get<bool>(env->lookup("c")) == true);
  }

  // --- Parent chain ---------------------------------------------------------

  TEST_CASE("lookup walks parent chain") {
    auto parent = std::make_shared<Environment>();
    parent->define("x", 10);

    auto child = std::make_shared<Environment>(parent);
    CHECK(std::get<int>(child->lookup("x")) == 10);
  }

  TEST_CASE("lookup walks multiple parents") {
    auto grandparent = std::make_shared<Environment>();
    grandparent->define("x", 1);

    auto parent = std::make_shared<Environment>(grandparent);
    auto child = std::make_shared<Environment>(parent);
    CHECK(std::get<int>(child->lookup("x")) == 1);
  }

  // --- Shadowing ------------------------------------------------------------

  TEST_CASE("child shadows parent binding") {
    auto parent = std::make_shared<Environment>();
    parent->define("x", 1);

    auto child = std::make_shared<Environment>(parent);
    child->define("x", 2);

    CHECK(std::get<int>(child->lookup("x")) == 2);
    CHECK(std::get<int>(parent->lookup("x")) == 1);
  }

  // --- Set ------------------------------------------------------------------

  TEST_CASE("set mutates binding in current scope") {
    auto env = std::make_shared<Environment>();
    env->define("x", 1);
    env->set("x", 99);
    CHECK(std::get<int>(env->lookup("x")) == 99);
  }

  TEST_CASE("set mutates binding in parent scope") {
    auto parent = std::make_shared<Environment>();
    parent->define("x", 1);

    auto child = std::make_shared<Environment>(parent);
    child->set("x", 99);

    CHECK(std::get<int>(parent->lookup("x")) == 99);
    CHECK(std::get<int>(child->lookup("x")) == 99);
  }

  TEST_CASE("set targets nearest binding when shadowed") {
    auto parent = std::make_shared<Environment>();
    parent->define("x", 1);

    auto child = std::make_shared<Environment>(parent);
    child->define("x", 2);

    child->set("x", 99);
    CHECK(std::get<int>(child->lookup("x")) == 99);
    CHECK(std::get<int>(parent->lookup("x")) == 1);
  }

  // --- Error cases ----------------------------------------------------------

  TEST_CASE("lookup throws on unbound variable") {
    auto env = std::make_shared<Environment>();
    CHECK_THROWS(env->lookup("nope"));
  }

  TEST_CASE("set throws on unbound variable") {
    auto env = std::make_shared<Environment>();
    CHECK_THROWS(env->set("nope", 1));
  }

  TEST_CASE("lookup throws when not in any parent") {
    auto parent = std::make_shared<Environment>();
    auto child = std::make_shared<Environment>(parent);
    CHECK_THROWS(child->lookup("nope"));
  }
}

//****************************************************************************
} // namespace blip
//****************************************************************************
