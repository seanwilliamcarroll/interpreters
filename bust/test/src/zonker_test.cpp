//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::Zonker
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <type_checker.hpp>
#include <variant>
#include <zonker.hpp>

#include <doctest/doctest.h>
#include <sstream>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.zonker") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program zonk_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    auto program = parser.parse();
    auto typed = TypeChecker{}(program);
    return Zonker{}(std::move(typed));
  }

  static bool is_concrete(const hir::TypeRegistry &registry, hir::TypeId id) {
    return !std::holds_alternative<hir::TypeVariable>(registry.get(id));
  }

  // --- Unifier state is consumed -------------------------------------------

  TEST_CASE("zonked program has no unifier state") {
    auto program = zonk_string("fn main() -> i64 { 42 }");
    CHECK_FALSE(program.m_unifier_state.has_value());
  }

  // --- No type variables survive -------------------------------------------

  TEST_CASE("function def types are concrete after zonking") {
    auto program = zonk_string("fn main() -> i64 { 42 }");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    CHECK(is_concrete(program.m_type_registry, func.m_signature.m_type));
    CHECK(is_concrete(program.m_type_registry, func.m_body.m_type));
  }

  TEST_CASE("let binding types are concrete after zonking") {
    auto program = zonk_string("fn main() -> i64 {\n"
                               "  let x: i64 = 10;\n"
                               "  x\n"
                               "}");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    CHECK(is_concrete(program.m_type_registry, let.m_variable.m_type));
    CHECK(is_concrete(program.m_type_registry, let.m_expression.m_type));
  }

  TEST_CASE("lambda with inferred parameter type is concrete after zonking") {
    auto program = zonk_string("fn main() -> i64 {\n"
                               "  let add_one = |x| { x + 1 };\n"
                               "  add_one(41)\n"
                               "}");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);

    // The let binding should have the lambda
    REQUIRE(!func.m_body.m_statements.empty());
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    CHECK(is_concrete(program.m_type_registry, let.m_variable.m_type));
    CHECK(is_concrete(program.m_type_registry, let.m_expression.m_type));

    // The lambda's function type should be concrete
    auto &lambda_expr = let.m_expression;
    auto &lambda =
        std::get<std::unique_ptr<hir::LambdaExpr>>(lambda_expr.m_expression);
    for (const auto &param : lambda->m_parameters) {
      CHECK(is_concrete(program.m_type_registry, param.m_type));
    }
    CHECK(is_concrete(program.m_type_registry, lambda->m_return_type));
  }

  // --- Resolved types are correct ------------------------------------------

  TEST_CASE("inferred lambda parameter resolves to correct type") {
    auto program = zonk_string("fn main() -> i64 {\n"
                               "  let add_one = |x| { x + 1 };\n"
                               "  add_one(41)\n"
                               "}");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
        let.m_expression.m_expression);

    REQUIRE(lambda->m_parameters.size() == 1);
    auto &param_type =
        program.m_type_registry.get(lambda->m_parameters[0].m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(param_type));
    CHECK(std::get<hir::PrimitiveTypeValue>(param_type).m_type ==
          PrimitiveType::I64);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
