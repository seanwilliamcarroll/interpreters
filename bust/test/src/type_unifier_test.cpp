//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
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

#include <hir/type_unifier.hpp>
#include <hir/type_visitors.hpp>
#include <hir/types.hpp>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.type_unifier") {

  // --- fresh type variables --------------------------------------------------

  TEST_CASE("new type variables get unique ids") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();
    CHECK(t0.m_id != t1.m_id);
    CHECK(t1.m_id != t2.m_id);
    CHECK(t0.m_id != t2.m_id);
  }

  TEST_CASE("unresolved type variable finds back to itself") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto result = unifier.find(t0);
    CHECK(std::holds_alternative<hir::TypeVariable>(result));
  }

  // --- unify(TypeVariable, concrete Type) ------------------------------------

  TEST_CASE("unify type variable with concrete type resolves it") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    unifier.unify(t0, i64_type);

    auto resolved = unifier.find(t0);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(resolved));
    CHECK(std::get<hir::PrimitiveTypeValue>(resolved).m_type ==
          PrimitiveType::I64);
  }

  TEST_CASE("unify concrete type with type variable (reversed arg order)") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();

    hir::Type bool_type = hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL};
    unifier.unify(bool_type, t0);

    auto resolved = unifier.find(t0);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(resolved));
    CHECK(std::get<hir::PrimitiveTypeValue>(resolved).m_type ==
          PrimitiveType::BOOL);
  }

  TEST_CASE("unify same type variable with same concrete type twice is ok") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    unifier.unify(t0, i64_type);
    CHECK_NOTHROW(unifier.unify(t0, i64_type));
  }

  TEST_CASE("unify type variable with conflicting concrete types throws") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    hir::Type bool_type = hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL};
    unifier.unify(t0, i64_type);
    CHECK_THROWS(unifier.unify(t0, bool_type));
  }

  // --- unify(TypeVariable, TypeVariable) -------------------------------------

  TEST_CASE("unify two unresolved type variables links them") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, t1);

    // Both still unresolved, but should share a root
    auto r0 = unifier.find(t0);
    auto r1 = unifier.find(t1);
    REQUIRE(std::holds_alternative<hir::TypeVariable>(r0));
    REQUIRE(std::holds_alternative<hir::TypeVariable>(r1));
    CHECK(std::get<hir::TypeVariable>(r0).m_id ==
          std::get<hir::TypeVariable>(r1).m_id);
  }

  TEST_CASE("unify two type variables then resolve one resolves both") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    unifier.unify(t0, t1);

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    unifier.unify(t0, i64_type);

    auto r0 = unifier.find(t0);
    auto r1 = unifier.find(t1);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(r0));
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(r1));
    CHECK(std::get<hir::PrimitiveTypeValue>(r0).m_type == PrimitiveType::I64);
    CHECK(std::get<hir::PrimitiveTypeValue>(r1).m_type == PrimitiveType::I64);
  }

  TEST_CASE("unify resolved variable with unresolved propagates type") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    hir::Type bool_type = hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL};
    unifier.unify(t0, bool_type);
    unifier.unify(t0, t1);

    auto resolved = unifier.find(t1);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(resolved));
    CHECK(std::get<hir::PrimitiveTypeValue>(resolved).m_type ==
          PrimitiveType::BOOL);
  }

  TEST_CASE("unify two variables both resolved to same type is ok") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    unifier.unify(t0, i64_type);
    unifier.unify(t1, i64_type);

    CHECK_NOTHROW(unifier.unify(t0, t1));
  }

  TEST_CASE("unify two variables resolved to different types throws") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    hir::Type bool_type = hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL};
    unifier.unify(t0, i64_type);
    unifier.unify(t1, bool_type);

    CHECK_THROWS(unifier.unify(t0, t1));
  }

  // --- transitive chains -----------------------------------------------------

  TEST_CASE("chain of three variables resolves transitively") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();

    unifier.unify(t0, t1);
    unifier.unify(t1, t2);

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    unifier.unify(t2, i64_type);

    auto r0 = unifier.find(t0);
    auto r1 = unifier.find(t1);
    auto r2 = unifier.find(t2);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(r0));
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(r1));
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(r2));
    CHECK(std::get<hir::PrimitiveTypeValue>(r0).m_type == PrimitiveType::I64);
    CHECK(std::get<hir::PrimitiveTypeValue>(r1).m_type == PrimitiveType::I64);
    CHECK(std::get<hir::PrimitiveTypeValue>(r2).m_type == PrimitiveType::I64);
  }

  TEST_CASE("resolve first in chain propagates to all") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();
    auto t2 = unifier.new_type_var();

    hir::Type bool_type = hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL};
    unifier.unify(t0, bool_type);
    unifier.unify(t0, t1);
    unifier.unify(t1, t2);

    auto r2 = unifier.find(t2);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(r2));
    CHECK(std::get<hir::PrimitiveTypeValue>(r2).m_type == PrimitiveType::BOOL);
  }

  // --- self unification ------------------------------------------------------

  TEST_CASE("unify type variable with itself is a no-op") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    CHECK_NOTHROW(unifier.unify(t0, t0));

    auto result = unifier.find(t0);
    CHECK(std::holds_alternative<hir::TypeVariable>(result));
  }

  // --- independent variables -------------------------------------------------

  TEST_CASE("independent type variables do not affect each other") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    unifier.unify(t0, i64_type);

    auto r1 = unifier.find(t1);
    CHECK(std::holds_alternative<hir::TypeVariable>(r1));
  }

} // TEST_SUITE

// --- FreeTypeVariableCollector tests ----------------------------------------

TEST_SUITE("bust.free_type_variable_collector") {

  TEST_CASE("collects type variable from bare TV") {
    hir::TypeUnifier unifier;
    auto tv = unifier.new_type_var();
    hir::Type type = tv;

    hir::FreeTypeVariableCollector collector{};
    std::visit(collector, type);

    CHECK(collector.m_free_type_variables.size() == 1);
    CHECK(collector.m_free_type_variables[0].m_id == tv.m_id);
  }

  TEST_CASE("collects nothing from concrete primitive") {
    hir::Type type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};

    hir::FreeTypeVariableCollector collector{};
    std::visit(collector, type);

    CHECK(collector.m_free_type_variables.empty());
  }

  TEST_CASE("collects nothing from NeverType") {
    hir::Type type = hir::NeverType{};

    hir::FreeTypeVariableCollector collector{};
    std::visit(collector, type);

    CHECK(collector.m_free_type_variables.empty());
  }

  TEST_CASE("collects TVs from inside FunctionType") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();
    auto t1 = unifier.new_type_var();

    // fn(?T0) -> ?T1
    std::vector<hir::Type> params;
    params.emplace_back(hir::clone_type(t0));
    hir::Type fn_type = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{}, std::move(params), hir::clone_type(t1)});

    hir::FreeTypeVariableCollector collector{};
    std::visit(collector, fn_type);

    CHECK(collector.m_free_type_variables.size() == 2);
  }

  TEST_CASE("collects only TVs not concrete parts of fn type") {
    // fn(i64) -> ?T1 — only ?T1 is a TV
    hir::TypeUnifier unifier;
    auto t1 = unifier.new_type_var();

    std::vector<hir::Type> params;
    params.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    hir::Type fn_type = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{}, std::move(params), hir::clone_type(t1)});

    hir::FreeTypeVariableCollector collector{};
    std::visit(collector, fn_type);

    CHECK(collector.m_free_type_variables.size() == 1);
    CHECK(collector.m_free_type_variables[0].m_id == t1.m_id);
  }
}

// --- Function type unification tests -----------------------------------------

TEST_SUITE("bust.type_unifier.function_types") {

  TEST_CASE("unify two identical concrete function types") {
    hir::TypeUnifier unifier;

    // fn(i64) -> bool
    auto make_fn = []() {
      std::vector<hir::Type> params;
      params.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
      return std::make_unique<hir::FunctionType>(
          hir::FunctionType{{},
                            std::move(params),
                            hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL}});
    };

    hir::Type fn_a = make_fn();
    hir::Type fn_b = make_fn();
    CHECK_NOTHROW(unifier.unify(fn_a, fn_b));
  }

  TEST_CASE("unify function types with different return types throws") {
    hir::TypeUnifier unifier;

    std::vector<hir::Type> params_a;
    params_a.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    hir::Type fn_a = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{},
                          std::move(params_a),
                          hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL}});

    std::vector<hir::Type> params_b;
    params_b.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    hir::Type fn_b = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{},
                          std::move(params_b),
                          hir::PrimitiveTypeValue{{}, PrimitiveType::I64}});

    CHECK_THROWS(unifier.unify(fn_a, fn_b));
  }

  TEST_CASE("unify function types with different arity throws") {
    hir::TypeUnifier unifier;

    // fn(i64) -> i64
    std::vector<hir::Type> params_a;
    params_a.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    hir::Type fn_a = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{},
                          std::move(params_a),
                          hir::PrimitiveTypeValue{{}, PrimitiveType::I64}});

    // fn(i64, i64) -> i64
    std::vector<hir::Type> params_b;
    params_b.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    params_b.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    hir::Type fn_b = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{},
                          std::move(params_b),
                          hir::PrimitiveTypeValue{{}, PrimitiveType::I64}});

    CHECK_THROWS(unifier.unify(fn_a, fn_b));
  }

  TEST_CASE("unify function type with type variable resolves it") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();

    // fn(i64) -> bool
    std::vector<hir::Type> params;
    params.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    hir::Type fn_type = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{},
                          std::move(params),
                          hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL}});

    unifier.unify(t0, fn_type);

    auto resolved = unifier.find(t0);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<hir::FunctionType>>(resolved));
  }

  TEST_CASE("unify function type containing type variables resolves params") {
    hir::TypeUnifier unifier;
    auto t0 = unifier.new_type_var();

    // fn(?T0) -> bool
    std::vector<hir::Type> params;
    params.emplace_back(hir::clone_type(t0));
    hir::Type fn_a = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{},
                          std::move(params),
                          hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL}});

    // fn(i64) -> bool
    std::vector<hir::Type> params_b;
    params_b.emplace_back(hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
    hir::Type fn_b = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{},
                          std::move(params_b),
                          hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL}});

    unifier.unify(fn_a, fn_b);

    // t0 should now be resolved to i64
    auto resolved = unifier.find(t0);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(resolved));
    CHECK(std::get<hir::PrimitiveTypeValue>(resolved).m_type ==
          PrimitiveType::I64);
  }

  TEST_CASE("unify two concrete mismatched types throws") {
    hir::TypeUnifier unifier;

    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    hir::Type bool_type = hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL};

    CHECK_THROWS(unifier.unify(i64_type, bool_type));
  }

  TEST_CASE("unify two identical concrete types is ok") {
    hir::TypeUnifier unifier;

    hir::Type i64_a = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    hir::Type i64_b = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};

    CHECK_NOTHROW(unifier.unify(i64_a, i64_b));
  }

  TEST_CASE("never type unifies with anything") {
    hir::TypeUnifier unifier;

    hir::Type never = hir::NeverType{};
    hir::Type i64_type = hir::PrimitiveTypeValue{{}, PrimitiveType::I64};
    hir::Type bool_type = hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL};

    CHECK_NOTHROW(unifier.unify(never, i64_type));
    CHECK_NOTHROW(unifier.unify(bool_type, never));
    CHECK_NOTHROW(unifier.unify(never, never));
  }
}

//****************************************************************************
} // namespace bust
//****************************************************************************
