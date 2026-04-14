//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::hir::TypeUnifier
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include "hir/environment.hpp"
#include <hir/free_type_variable_collector.hpp>
#include <hir/type_unifier.hpp>
#include <hir/type_variable_substituter.hpp>
#include <hir/types.hpp>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.type_unifier") {

  // --- fresh type variables --------------------------------------------------

  TEST_CASE("new type variables get unique ids") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();
    CHECK(t0.m_id != t1.m_id);
    CHECK(t1.m_id != t2.m_id);
    CHECK(t0.m_id != t2.m_id);
  }

  TEST_CASE("unresolved type variable finds back to itself") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto result = unifier.find(t0);
    CHECK(std::holds_alternative<hir::TypeVariable>(type_registry.get(result)));
  }

  // --- unify(TypeVariable, concrete Type) ------------------------------------

  TEST_CASE("unify type variable with concrete type resolves it") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_i64);

    auto resolved = unifier.find(t0);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(
        type_registry.get(resolved)));
    CHECK(resolved == type_registry.m_i64);
  }

  TEST_CASE("unify concrete type with type variable (reversed arg order)") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();

    unifier.unify(type_registry.m_bool, t0);

    auto resolved = unifier.find(t0);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(
        type_registry.get(resolved)));
    CHECK(resolved == type_registry.m_bool);
  }

  TEST_CASE("unify same type variable with same concrete type twice is ok") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_i64);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i64));
  }

  TEST_CASE("unify type variable with conflicting concrete types throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_i64);
    CHECK_THROWS(unifier.unify(t0, type_registry.m_bool));
  }

  // --- unify(TypeVariable, TypeVariable) -------------------------------------

  TEST_CASE("unify two unresolved type variables links them") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, t1);

    // Both still unresolved, but should share a root
    auto r0 = unifier.find(t0);
    auto r1 = unifier.find(t1);
    REQUIRE(std::holds_alternative<hir::TypeVariable>(type_registry.get(r0)));
    REQUIRE(std::holds_alternative<hir::TypeVariable>(type_registry.get(r1)));
    CHECK(std::get<hir::TypeVariable>(type_registry.get(r0)).m_id ==
          std::get<hir::TypeVariable>(type_registry.get(r1)).m_id);
  }

  TEST_CASE("unify two type variables then resolve one resolves both") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, t1);

    unifier.unify(t0, type_registry.m_i64);

    auto r0 = unifier.find(t0);
    auto r1 = unifier.find(t1);
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r0)));
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r1)));
    CHECK(r0 == type_registry.m_i64);
    CHECK(r1 == type_registry.m_i64);
  }

  TEST_CASE("unify resolved variable with unresolved propagates type") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_bool);
    unifier.unify(t0, t1);

    auto resolved = unifier.find(t1);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(
        type_registry.get(resolved)));
    CHECK(resolved == type_registry.m_bool);
  }

  TEST_CASE("unify two variables both resolved to same type is ok") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_i64);
    unifier.unify(t1, type_registry.m_i64);

    CHECK_NOTHROW(unifier.unify(t0, t1));
  }

  TEST_CASE("unify two variables resolved to different types throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_i64);
    unifier.unify(t1, type_registry.m_bool);

    CHECK_THROWS(unifier.unify(t0, t1));
  }

  // --- transitive chains -----------------------------------------------------

  TEST_CASE("chain of three variables resolves transitively") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();

    unifier.unify(t0, t1);
    unifier.unify(t1, t2);

    unifier.unify(t2, type_registry.m_i64);

    auto r0 = unifier.find(t0);
    auto r1 = unifier.find(t1);
    auto r2 = unifier.find(t2);
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r0)));
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r1)));
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r2)));
    CHECK(r0 == type_registry.m_i64);
    CHECK(r1 == type_registry.m_i64);
    CHECK(r2 == type_registry.m_i64);
  }

  TEST_CASE("resolve first in chain propagates to all") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_bool);
    unifier.unify(t0, t1);
    unifier.unify(t1, t2);

    auto r2 = unifier.find(t2);
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r2)));
    CHECK(r2 == type_registry.m_bool);
  }

  // --- self unification ------------------------------------------------------

  TEST_CASE("unify type variable with itself is a no-op") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    CHECK_NOTHROW(unifier.unify(t0, t0));

    auto result = unifier.find(t0);
    CHECK(std::holds_alternative<hir::TypeVariable>(type_registry.get(result)));
  }

  // --- independent variables -------------------------------------------------

  TEST_CASE("independent type variables do not affect each other") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, type_registry.m_i64);

    auto r1 = unifier.find(t1);
    CHECK(std::holds_alternative<hir::TypeVariable>(type_registry.get(r1)));
  }

} // TEST_SUITE

// --- FreeTypeVariableCollector tests ----------------------------------------

TEST_SUITE("bust.free_type_variable_collector") {

  TEST_CASE("collects type variable from bare TV") {
    auto context = hir::Context{};
    auto tv = context.m_type_unifier.new_type_var();
    hir::TypeKind type = context.m_type_registry.as_type_variable(tv);

    hir::FreeTypeVariableCollector collector{context};
    std::visit(collector, type);

    CHECK(collector.m_free_type_variables.size() == 1);
    CHECK(collector.m_free_type_variables[0].m_id == tv.m_id);
  }

  TEST_CASE("collects nothing from concrete primitive") {
    hir::TypeKind type = hir::PrimitiveTypeValue{PrimitiveType::I64};

    auto context = hir::Context{};
    hir::FreeTypeVariableCollector collector{context};
    std::visit(collector, type);

    CHECK(collector.m_free_type_variables.empty());
  }

  TEST_CASE("collects nothing from NeverType") {
    hir::TypeKind type = hir::NeverType{};
    auto context = hir::Context{};
    hir::FreeTypeVariableCollector collector{context};
    std::visit(collector, type);

    CHECK(collector.m_free_type_variables.empty());
  }

  TEST_CASE("collects TVs from inside FunctionType") {
    auto context = hir::Context{};
    auto t0 = context.m_type_unifier.new_type_var();
    auto t1 = context.m_type_unifier.new_type_var();

    // fn(?T0) -> ?T1
    std::vector<hir::TypeId> params;
    params.emplace_back(t0);
    hir::TypeKind fn_type = hir::FunctionType{std::move(params), t1};

    hir::FreeTypeVariableCollector collector{context};
    std::visit(collector, fn_type);

    CHECK(collector.m_free_type_variables.size() == 2);
  }

  TEST_CASE("collects only TVs not concrete parts of fn type") {
    // fn(i64) -> ?T1 — only ?T1 is a TV
    auto context = hir::Context{};
    auto t1 = context.m_type_unifier.new_type_var();

    std::vector<hir::TypeId> params;
    params.emplace_back(context.m_type_registry.m_i64);
    hir::TypeKind fn_type = hir::FunctionType{std::move(params), t1};

    hir::FreeTypeVariableCollector collector{context};
    std::visit(collector, fn_type);

    CHECK(collector.m_free_type_variables.size() == 1);
    CHECK(collector.m_free_type_variables[0].m_id == t1.m_id);
  }
}

// --- Function type unification tests -----------------------------------------

TEST_SUITE("bust.type_unifier.function_types") {

  TEST_CASE("unify two identical concrete function types") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};

    // fn(i64) -> bool
    auto make_fn = [&]() {
      std::vector<hir::TypeId> params;
      params.emplace_back(type_registry.m_i64);
      return hir::FunctionType{std::move(params), type_registry.m_bool};
    };

    hir::TypeKind fn_a = make_fn();
    hir::TypeKind fn_b = make_fn();
    CHECK_NOTHROW(
        unifier.unify(type_registry.intern(fn_a), type_registry.intern(fn_b)));
  }

  TEST_CASE("unify function types with different return types throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};

    std::vector<hir::TypeId> params_a;
    params_a.emplace_back(type_registry.m_i64);
    hir::TypeKind fn_a =
        hir::FunctionType{std::move(params_a), type_registry.m_bool};

    std::vector<hir::TypeId> params_b;
    params_b.emplace_back(type_registry.m_i64);
    hir::TypeKind fn_b =
        hir::FunctionType{std::move(params_b), type_registry.m_i64};

    CHECK_THROWS(
        unifier.unify(type_registry.intern(fn_a), type_registry.intern(fn_b)));
  }

  TEST_CASE("unify function types with different arity throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};

    // fn(i64) -> i64
    std::vector<hir::TypeId> params_a;
    params_a.emplace_back(type_registry.m_i64);
    hir::TypeKind fn_a =
        hir::FunctionType{std::move(params_a), type_registry.m_i64};

    // fn(i64, i64) -> i64
    std::vector<hir::TypeId> params_b;
    params_b.emplace_back(type_registry.m_i64);
    params_b.emplace_back(type_registry.m_i64);
    hir::TypeKind fn_b =
        hir::FunctionType{std::move(params_b), type_registry.m_i64};

    CHECK_THROWS(
        unifier.unify(type_registry.intern(fn_a), type_registry.intern(fn_b)));
  }

  TEST_CASE("unify function type with type variable resolves it") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();

    // fn(i64) -> bool
    std::vector<hir::TypeId> params;
    params.emplace_back(type_registry.m_i64);
    hir::TypeKind fn_type =
        hir::FunctionType{std::move(params), type_registry.m_bool};

    unifier.unify(t0, type_registry.intern(fn_type));

    auto resolved = unifier.find(t0);
    REQUIRE(
        std::holds_alternative<hir::FunctionType>(type_registry.get(resolved)));
  }

  TEST_CASE("unify function type containing type variables resolves params") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();

    // fn(?T0) -> bool
    std::vector<hir::TypeId> params;
    params.emplace_back(t0);
    hir::TypeKind fn_a =
        hir::FunctionType{std::move(params), type_registry.m_bool};

    // fn(i64) -> bool
    std::vector<hir::TypeId> params_b;
    params_b.emplace_back(type_registry.m_i64);
    hir::TypeKind fn_b =
        hir::FunctionType{std::move(params_b), type_registry.m_bool};

    unifier.unify(type_registry.intern(fn_a), type_registry.intern(fn_b));

    // t0 should now be resolved to i64
    auto resolved = unifier.find(t0);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(
        type_registry.get(resolved)));
    CHECK(resolved == type_registry.m_i64);
  }

  TEST_CASE("unify two concrete mismatched types throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};

    CHECK_THROWS(unifier.unify(type_registry.m_i64, type_registry.m_bool));
  }

  TEST_CASE("unify two identical concrete types is ok") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};

    hir::TypeKind i64_a = hir::PrimitiveTypeValue{PrimitiveType::I64};
    hir::TypeKind i64_b = hir::PrimitiveTypeValue{PrimitiveType::I64};

    CHECK_NOTHROW(unifier.unify(type_registry.intern(i64_a),
                                type_registry.intern(i64_b)));
  }

  TEST_CASE("never type unifies with anything") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};

    hir::TypeKind never = hir::NeverType{};

    CHECK_NOTHROW(
        unifier.unify(type_registry.intern(never), type_registry.m_i64));
    CHECK_NOTHROW(
        unifier.unify(type_registry.m_bool, type_registry.intern(never)));
    CHECK_NOTHROW(unifier.unify(type_registry.intern(never),
                                type_registry.intern(never)));
  }
}

// --- Type class constraint tests ---------------------------------------------

TEST_SUITE("bust.type_unifier.constraints") {

  // --- constrain() basic behavior -------------------------------------------

  TEST_CASE("constrain unresolved variable stores constraint") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    CHECK_NOTHROW(unifier.constrain(t0, PrimitiveTypeClass::NUMERIC));
  }

  TEST_CASE("constrain then resolve with compatible type succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i64));
  }

  TEST_CASE("constrain then resolve with i8 succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i8));
  }

  TEST_CASE("constrain then resolve with i32 succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i32));
  }

  TEST_CASE("constrain NUMERIC then resolve with bool throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_THROWS(unifier.unify(t0, type_registry.m_bool));
  }

  TEST_CASE("constrain NUMERIC then resolve with char throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_THROWS(unifier.unify(t0, type_registry.m_char));
  }

  TEST_CASE("constrain NUMERIC then resolve with unit throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_THROWS(unifier.unify(t0, type_registry.m_unit));
  }

  TEST_CASE("constrain BOOL then resolve with bool succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::BOOL);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_bool));
  }

  TEST_CASE("constrain BOOL then resolve with i64 throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::BOOL);
    CHECK_THROWS(unifier.unify(t0, type_registry.m_i64));
  }

  TEST_CASE("constrain COMPARABLE then resolve with i64 succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i64));
  }

  TEST_CASE("constrain COMPARABLE then resolve with bool succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_bool));
  }

  TEST_CASE("constrain COMPARABLE then resolve with char succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE);
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_char));
  }

  TEST_CASE("constrain COMPARABLE then resolve with unit throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE);
    CHECK_THROWS(unifier.unify(t0, type_registry.m_unit));
  }

  // --- constrain after resolve (reverse order) ------------------------------

  TEST_CASE("resolve then constrain with compatible class succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.unify(t0, type_registry.m_i64);
    CHECK_NOTHROW(unifier.constrain(t0, PrimitiveTypeClass::NUMERIC));
  }

  TEST_CASE("resolve then constrain with incompatible class throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.unify(t0, type_registry.m_bool);
    CHECK_THROWS(unifier.constrain(t0, PrimitiveTypeClass::NUMERIC));
  }

  TEST_CASE("resolve to char then constrain NUMERIC throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.unify(t0, type_registry.m_char);
    CHECK_THROWS(unifier.constrain(t0, PrimitiveTypeClass::NUMERIC));
  }

  TEST_CASE("resolve to char then constrain COMPARABLE succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.unify(t0, type_registry.m_char);
    CHECK_NOTHROW(unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE));
  }

  // --- multiple constraints on same variable --------------------------------

  TEST_CASE("same constraint twice is idempotent") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.constrain(t0, PrimitiveTypeClass::NUMERIC));
  }

  TEST_CASE("NUMERIC then COMPARABLE keeps NUMERIC (tighter)") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE));
    // NUMERIC is tighter; char should still be rejected
    CHECK_THROWS(unifier.unify(t0, type_registry.m_char));
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i32));
  }

  TEST_CASE("COMPARABLE then NUMERIC tightens to NUMERIC") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE);
    CHECK_NOTHROW(unifier.constrain(t0, PrimitiveTypeClass::NUMERIC));
    CHECK_THROWS(unifier.unify(t0, type_registry.m_bool));
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i64));
  }

  TEST_CASE("NUMERIC then BOOL throws (disjoint)") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    CHECK_THROWS(unifier.constrain(t0, PrimitiveTypeClass::BOOL));
  }

  TEST_CASE("BOOL then NUMERIC throws (disjoint)") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::BOOL);
    CHECK_THROWS(unifier.constrain(t0, PrimitiveTypeClass::NUMERIC));
  }

  // --- constraints through unify(TV, TV) ------------------------------------

  TEST_CASE(
      "constrained var unified with unconstrained propagates constraint") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    unifier.unify(t0, t1);
    // t1 should now inherit NUMERIC; resolving to bool should fail
    CHECK_THROWS(unifier.unify(t1, type_registry.m_bool));
    CHECK_NOTHROW(unifier.unify(t1, type_registry.m_i64));
  }

  TEST_CASE("unconstrained unified with constrained inherits constraint") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t1, PrimitiveTypeClass::BOOL);
    unifier.unify(t0, t1);
    CHECK_THROWS(unifier.unify(t0, type_registry.m_i64));
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_bool));
  }

  TEST_CASE("two NUMERIC vars unified then resolved") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    unifier.constrain(t1, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.unify(t0, t1));
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i8));
    // Both should resolve
    auto r0 = unifier.find(t0);
    auto r1 = unifier.find(t1);
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r0)));
    REQUIRE(
        std::holds_alternative<hir::PrimitiveTypeValue>(type_registry.get(r1)));
    CHECK(r0 == type_registry.m_i8);
    CHECK(r1 == type_registry.m_i8);
  }

  TEST_CASE("NUMERIC and COMPARABLE vars unified keeps NUMERIC") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    unifier.constrain(t1, PrimitiveTypeClass::COMPARABLE);
    CHECK_NOTHROW(unifier.unify(t0, t1));
    // Merged constraint should be NUMERIC (tighter)
    CHECK_THROWS(unifier.unify(t0, type_registry.m_char));
    CHECK_NOTHROW(unifier.unify(t0, type_registry.m_i32));
  }

  TEST_CASE("COMPARABLE and NUMERIC vars unified keeps NUMERIC") {
    // Same as above but reversed unify order
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::COMPARABLE);
    unifier.constrain(t1, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.unify(t0, t1));
    CHECK_THROWS(unifier.unify(t1, type_registry.m_bool));
  }

  TEST_CASE("NUMERIC and BOOL vars unified throws (disjoint)") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    unifier.constrain(t1, PrimitiveTypeClass::BOOL);
    CHECK_THROWS(unifier.unify(t0, t1));
  }

  // --- constraints with concrete type on one side ---------------------------

  TEST_CASE("constrained var unified with concrete-resolved var succeeds") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    unifier.unify(t1, type_registry.m_i64);
    CHECK_NOTHROW(unifier.unify(t0, t1));
  }

  TEST_CASE("constrained var unified with incompatible concrete var throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    unifier.unify(t1, type_registry.m_bool);
    CHECK_THROWS(unifier.unify(t0, t1));
  }

  TEST_CASE("concrete-resolved var unified with constrained var succeeds") {
    // Reversed: concrete on left, constraint on right
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.unify(t0, type_registry.m_i32);
    unifier.constrain(t1, PrimitiveTypeClass::NUMERIC);
    CHECK_NOTHROW(unifier.unify(t0, t1));
  }

  TEST_CASE("concrete char unified with NUMERIC-constrained var throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.unify(t0, type_registry.m_char);
    unifier.constrain(t1, PrimitiveTypeClass::NUMERIC);
    CHECK_THROWS(unifier.unify(t0, t1));
  }

  // --- transitive chains with constraints -----------------------------------

  TEST_CASE("constraint propagates through chain of unified vars") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();
    unifier.constrain(t0, PrimitiveTypeClass::NUMERIC);
    unifier.unify(t0, t1);
    unifier.unify(t1, t2);
    // t2 should have inherited NUMERIC
    CHECK_THROWS(unifier.unify(t2, type_registry.m_bool));
    CHECK_NOTHROW(unifier.unify(t2, type_registry.m_i64));
  }

  TEST_CASE("constraint applied to end of chain affects whole chain") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();
    unifier.unify(t0, t1);
    unifier.unify(t1, t2);
    unifier.constrain(t2, PrimitiveTypeClass::BOOL);
    // Resolving t0 to i64 should fail
    CHECK_THROWS(unifier.unify(t0, type_registry.m_i64));
  }

  // --- constrain after unify with concrete ----------------------------------

  TEST_CASE("constrain after chain is resolved checks concrete type") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.unify(t0, t1);
    unifier.unify(t0, type_registry.m_i64);
    // Adding compatible constraint after resolution is fine
    CHECK_NOTHROW(unifier.constrain(t1, PrimitiveTypeClass::NUMERIC));
  }

  TEST_CASE("constrain after chain is resolved with wrong class throws") {
    hir::TypeRegistry type_registry{};
    hir::TypeUnifier unifier{type_registry};
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    unifier.unify(t0, t1);
    unifier.unify(t0, type_registry.m_i64);
    CHECK_THROWS(unifier.constrain(t1, PrimitiveTypeClass::BOOL));
  }
}

//****************************************************************************
} // namespace bust
//****************************************************************************
