//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::Monomorpher
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include "pipeline.hpp"
#include "validate_main.hpp"
#include <hir/dump.hpp>
#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <lexer.hpp>
#include <monomorpher.hpp>
#include <parser.hpp>
#include <type_checker.hpp>

#include <doctest/doctest.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.monomorpher") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program mono_string_impl(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{},
                        Monomorpher{});
  }

  // Convenience macro: declares `var` as the mono'd program for `src`,
  // and attaches an HIR dump via doctest's INFO so the tree is printed
  // alongside any subsequent CHECK/REQUIRE failure in the same scope.
  // Must be a macro (not a helper) because INFO is scope-bound — it only
  // fires for assertions in the block where it's declared.
#define MONO_STRING(var, src)                                                  \
  auto(var) = mono_string_impl(src);                                           \
  INFO("HIR:\n" << hir::Dumper::dump(var))

  static bool is_concrete(const hir::TypeRegistry &registry, hir::TypeId id) {
    return !std::holds_alternative<hir::TypeVariable>(registry.get(id));
  }

  // Collect all let bindings in a block whose name starts with the given
  // prefix. Useful for finding specializations (e.g. "id__bi…").
  static std::vector<const hir::LetBinding *> find_let_bindings_with_prefix(
      const hir::Block &block, const std::string &prefix) {
    std::vector<const hir::LetBinding *> result;
    for (const auto &stmt : block.m_statements) {
      if (const auto *let = std::get_if<hir::LetBinding>(&stmt)) {
        if (let->m_variable.m_name.rfind(prefix, 0) == 0) {
          result.push_back(let);
        }
      }
    }
    return result;
  }

  // Collect the callee-identifier name from every top-level call expression
  // statement in a block.
  static std::vector<std::string> collect_call_callee_names(
      const hir::Block &block) {
    std::vector<std::string> names;
    for (const auto &stmt : block.m_statements) {
      if (const auto *expr = std::get_if<hir::Expression>(&stmt)) {
        if (const auto *call = std::get_if<std::unique_ptr<hir::CallExpr>>(
                &expr->m_expression)) {
          const auto &callee_id =
              std::get<hir::Identifier>((*call)->m_callee.m_expression);
          names.push_back(callee_id.m_name);
        }
      }
    }
    return names;
  }

  // --- Structure preservation ----------------------------------------------

  TEST_CASE("non-polymorphic program preserves top items") {
    MONO_STRING(program, "fn main() -> i64 { 42 }");
    REQUIRE(program.m_top_items.size() == 1);
    CHECK(std::holds_alternative<hir::FunctionDef>(program.m_top_items[0]));
  }

  TEST_CASE("FunctionDef signature preserved") {
    MONO_STRING(program, "fn main() -> i64 { 42 }");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    CHECK(func.m_signature.m_function_id == "main");
  }

  TEST_CASE("extern function declaration passes through unchanged") {
    MONO_STRING(program, "extern fn putchar(c: i32) -> i32;\n"
                         "fn main() -> i64 { 0 }");
    REQUIRE(program.m_top_items.size() == 2);
    REQUIRE(std::holds_alternative<hir::ExternFunctionDeclaration>(
        program.m_top_items[0]));
    auto &ext =
        std::get<hir::ExternFunctionDeclaration>(program.m_top_items[0]);
    CHECK(ext.m_signature.m_function_id == "putchar");
  }

  TEST_CASE("non-polymorphic let binding passes through with original name") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let x: i64 = 10;\n"
                         "  x\n"
                         "}");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    REQUIRE(func.m_body.m_statements.size() == 1);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    CHECK(let.m_variable.m_name == "x");
  }

  TEST_CASE("body of non-polymorphic function preserved") {
    MONO_STRING(program, "fn main() -> i64 { 42 }");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<hir::LiteralI64>(
        func.m_body.m_final_expression.value().m_expression));
  }

  // --- Polymorphic fanout --------------------------------------------------

  TEST_CASE("polymorphic let used at one type produces one specialization") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    CHECK(specializations.size() == 1);
  }

  TEST_CASE("polymorphic let used at two types produces two specializations") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42);\n"
                         "  id(true);\n"
                         "  0\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    CHECK(specializations.size() == 2);

    std::set<std::string> names;
    for (const auto *s : specializations) {
      names.insert(s->m_variable.m_name);
    }
    CHECK(names.size() == 2);
  }

  TEST_CASE(
      "polymorphic let used at same type twice produces one specialization") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(1);\n"
                         "  id(2)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    CHECK(specializations.size() == 1);
  }

  TEST_CASE("original polymorphic binding name does not survive") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    for (const auto &stmt : func.m_body.m_statements) {
      if (const auto *let = std::get_if<hir::LetBinding>(&stmt)) {
        CHECK(let->m_variable.m_name != "id");
      }
    }
  }

  // --- Specialization identity --------------------------------------------

  TEST_CASE("specialization has mangled name with binding-id marker") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specializations.size() == 1);
    CHECK(specializations[0]->m_variable.m_name != "id");
    CHECK(specializations[0]->m_variable.m_name.find("__bi") !=
          std::string::npos);
  }

  TEST_CASE("two specializations have different binding ids") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42);\n"
                         "  id(true);\n"
                         "  0\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specializations.size() == 2);
    CHECK(specializations[0]->m_variable.m_id !=
          specializations[1]->m_variable.m_id);
  }

  // --- Types are concrete inside specializations --------------------------

  TEST_CASE("specialization's parameter types are concrete") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specializations.size() == 1);
    const auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
        specializations[0]->m_expression.m_expression);
    for (const auto &param : lambda->m_parameters) {
      CHECK(is_concrete(program.m_type_registry, param.m_type));
    }
  }

  TEST_CASE("specialization's return type is concrete") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specializations.size() == 1);
    const auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
        specializations[0]->m_expression.m_expression);
    CHECK(is_concrete(program.m_type_registry, lambda->m_return_type));
  }

  TEST_CASE("specialization's bound-name type is concrete") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specializations.size() == 1);
    CHECK(is_concrete(program.m_type_registry,
                      specializations[0]->m_variable.m_type));
    CHECK(is_concrete(program.m_type_registry,
                      specializations[0]->m_expression.m_type));
  }

  TEST_CASE("two specializations have structurally different types") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42);\n"
                         "  id(true);\n"
                         "  0\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specializations.size() == 2);
    CHECK(specializations[0]->m_variable.m_type !=
          specializations[1]->m_variable.m_type);
  }

  // --- Call-site identifier rewriting -------------------------------------

  TEST_CASE("call-site identifier is rewritten to specialization name") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    const auto &final_expr = func.m_body.m_final_expression.value();
    const auto &call =
        std::get<std::unique_ptr<hir::CallExpr>>(final_expr.m_expression);
    const auto &callee_id =
        std::get<hir::Identifier>(call->m_callee.m_expression);
    CHECK(callee_id.m_name != "id");
    CHECK(callee_id.m_name.find("__bi") != std::string::npos);
  }

  TEST_CASE("call-site identifier id matches the specialization's fresh id") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specializations = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specializations.size() == 1);

    REQUIRE(func.m_body.m_final_expression.has_value());
    const auto &final_expr = func.m_body.m_final_expression.value();
    const auto &call =
        std::get<std::unique_ptr<hir::CallExpr>>(final_expr.m_expression);
    const auto &callee_id =
        std::get<hir::Identifier>(call->m_callee.m_expression);

    CHECK(callee_id.m_id == specializations[0]->m_variable.m_id);
    CHECK(callee_id.m_name == specializations[0]->m_variable.m_name);
  }

  TEST_CASE("two call sites at different types resolve to different "
            "specializations") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42);\n"
                         "  id(true);\n"
                         "  0\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto callee_names = collect_call_callee_names(func.m_body);
    REQUIRE(callee_names.size() == 2);
    CHECK(callee_names[0] != callee_names[1]);
  }

  TEST_CASE(
      "two call sites at the same type resolve to the same specialization") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(1);\n"
                         "  id(2)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto callee_names = collect_call_callee_names(func.m_body);
    // Only the first call appears as a statement; the second is the final
    // expression. Collect both.
    std::vector<std::string> all_names = callee_names;
    REQUIRE(func.m_body.m_final_expression.has_value());
    const auto &final_expr = func.m_body.m_final_expression.value();
    const auto &call =
        std::get<std::unique_ptr<hir::CallExpr>>(final_expr.m_expression);
    const auto &callee_id =
        std::get<hir::Identifier>(call->m_callee.m_expression);
    all_names.push_back(callee_id.m_name);

    REQUIRE(all_names.size() == 2);
    CHECK(all_names[0] == all_names[1]);
  }

  // --- Richer programs -----------------------------------------------------

  TEST_CASE(
      "polymorphic higher-order function specializes with concrete types") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let apply = |f, x| { f(x) };\n"
                         "  let double_it = |x: i64| { x + x };\n"
                         "  apply(double_it, 5)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto apply_specs = find_let_bindings_with_prefix(func.m_body, "apply");
    REQUIRE(apply_specs.size() >= 1);
    for (const auto *spec : apply_specs) {
      const auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
          spec->m_expression.m_expression);
      for (const auto &param : lambda->m_parameters) {
        CHECK(is_concrete(program.m_type_registry, param.m_type));
      }
      CHECK(is_concrete(program.m_type_registry, lambda->m_return_type));
    }
  }

  TEST_CASE("non-polymorphic helper function is left alone") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let add_one = |x: i64| { x + 1 };\n"
                         "  add_one(41)\n"
                         "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    // The non-polymorphic let goes through the no-records path and should
    // keep its original name.
    bool found_add_one = false;
    for (const auto &stmt : func.m_body.m_statements) {
      if (const auto *let = std::get_if<hir::LetBinding>(&stmt)) {
        if (let->m_variable.m_name == "add_one") {
          found_add_one = true;
        }
      }
    }
    CHECK(found_add_one);
  }

  // --- Program-level post-conditions --------------------------------------

  TEST_CASE("instantiation records are cleared after monomorphization") {
    MONO_STRING(program, "fn main() -> i64 {\n"
                         "  let id = |x| { x };\n"
                         "  id(42)\n"
                         "}");
    CHECK(program.m_instantiation_records.empty());
  }

  TEST_CASE("unifier state is still present after monomorphization") {
    // Unifier state is consumed by the ZIR Lowerer, not by mono.
    MONO_STRING(program, "fn main() -> i64 { 42 }");
    CHECK(program.m_unifier_state.has_value());
  }

  TEST_CASE("monomorpher throws without unifier state") {
    hir::Program program;
    CHECK_THROWS(Monomorpher{}(std::move(program)));
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
