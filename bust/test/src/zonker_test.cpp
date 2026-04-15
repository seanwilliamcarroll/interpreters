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
#include <monomorpher.hpp>
#include <parser.hpp>
#include <type_checker.hpp>
#include <variant>
#include <zonker.hpp>

#include <doctest/doctest.h>
#include <set>
#include <sstream>
#include <string>

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

  // Full production pipeline: lex -> parse -> typecheck -> mono -> zonk.
  // Use for polymorphic programs whose type variables are only resolved
  // once monomorphization has fanned out specializations.
  static hir::Program mono_zonk_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    auto program = parser.parse();
    auto typed = TypeChecker{}(program);
    auto monoed = Monomorpher{}(std::move(typed));
    return Zonker{}(std::move(monoed));
  }

  static bool is_concrete(const hir::TypeRegistry &registry, hir::TypeId id) {
    return !std::holds_alternative<hir::TypeVariable>(registry.get(id));
  }

  // Recursively check that every TypeId reachable from an expression (and
  // its sub-expressions / nested blocks) resolves to a non-TypeVariable.
  static void require_expression_concrete(const hir::TypeRegistry &registry,
                                          const hir::Expression &expr);

  static void require_block_concrete(const hir::TypeRegistry &registry,
                                     const hir::Block &block) {
    CHECK(is_concrete(registry, block.m_type));
    for (const auto &stmt : block.m_statements) {
      if (const auto *expr = std::get_if<hir::Expression>(&stmt)) {
        require_expression_concrete(registry, *expr);
      } else if (const auto *let = std::get_if<hir::LetBinding>(&stmt)) {
        CHECK(is_concrete(registry, let->m_variable.m_type));
        require_expression_concrete(registry, let->m_expression);
      }
    }
    if (block.m_final_expression.has_value()) {
      require_expression_concrete(registry, block.m_final_expression.value());
    }
  }

  static void require_expression_concrete(const hir::TypeRegistry &registry,
                                          const hir::Expression &expr) {
    CHECK(is_concrete(registry, expr.m_type));
    std::visit(
        [&](const auto &e) {
          using T = std::decay_t<decltype(e)>;
          if constexpr (std::is_same_v<T, hir::Identifier>) {
            CHECK(is_concrete(registry, e.m_type));
          } else if constexpr (std::is_same_v<
                                   T, std::unique_ptr<hir::LambdaExpr>>) {
            for (const auto &param : e->m_parameters) {
              CHECK(is_concrete(registry, param.m_type));
            }
            CHECK(is_concrete(registry, e->m_return_type));
            require_block_concrete(registry, e->m_body);
          } else if constexpr (std::is_same_v<T,
                                              std::unique_ptr<hir::CallExpr>>) {
            require_expression_concrete(registry, e->m_callee);
            for (const auto &arg : e->m_arguments) {
              require_expression_concrete(registry, arg);
            }
          } else if constexpr (std::is_same_v<T, std::unique_ptr<hir::Block>>) {
            require_block_concrete(registry, *e);
          } else if constexpr (std::is_same_v<T,
                                              std::unique_ptr<hir::IfExpr>>) {
            require_expression_concrete(registry, e->m_condition);
            require_block_concrete(registry, e->m_then_block);
            if (e->m_else_block.has_value()) {
              require_block_concrete(registry, e->m_else_block.value());
            }
          } else if constexpr (std::is_same_v<
                                   T, std::unique_ptr<hir::BinaryExpr>>) {
            require_expression_concrete(registry, e->m_lhs);
            require_expression_concrete(registry, e->m_rhs);
          } else if constexpr (
              std::is_same_v<T, std::unique_ptr<hir::UnaryExpr>> ||
              std::is_same_v<T, std::unique_ptr<hir::ReturnExpr>> ||
              std::is_same_v<T, std::unique_ptr<hir::CastExpr>>) {
            require_expression_concrete(registry, e->m_expression);
          } else {
            // Literals carry their type statically — expr.m_type already
            // checked above.
          }
        },
        expr.m_expression);
  }

  // Collect every let binding (in any block reachable from a function body)
  // whose name begins with `prefix`. Specialization names look like
  // "<orig>__bi<id>__<type-mangle>", so prefix-matching on the original
  // name is the natural way to find them.
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

  // --- Extern function declarations -----------------------------------------

  TEST_CASE("extern function declaration types are concrete after zonking") {
    auto program = zonk_string("extern fn putchar(c: i32) -> i32;\n"
                               "fn main() -> i64 { 0 }");
    REQUIRE(program.m_top_items.size() == 2);
    auto &ext =
        std::get<hir::ExternFunctionDeclaration>(program.m_top_items[0]);
    CHECK(is_concrete(program.m_type_registry, ext.m_signature.m_type));
  }

  TEST_CASE("extern function survives full pipeline with call site") {
    auto program = zonk_string("extern fn putchar(c: i32) -> i32;\n"
                               "fn main() -> i64 {\n"
                               "  putchar(72 as i32);\n"
                               "  0\n"
                               "}");
    REQUIRE(program.m_top_items.size() == 2);
    auto &ext =
        std::get<hir::ExternFunctionDeclaration>(program.m_top_items[0]);
    CHECK(ext.m_signature.m_function_id == "putchar");

    auto &func = std::get<hir::FunctionDef>(program.m_top_items[1]);
    CHECK(is_concrete(program.m_type_registry, func.m_body.m_type));
  }

  // --- Polymorphic programs (full mono + zonk pipeline) -------------------
  //
  // These exercises all feed through Monomorpher before zonking. Pre-mono,
  // polymorphic lets would leave type variables the unifier could never
  // ground (because there is no single concrete type to ground them at).
  // Post-mono, each call site has its own specialization at a concrete
  // type, so zonking should succeed and produce a fully concrete program.

  TEST_CASE("polymorphic identity: single-use program is fully concrete") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(42)\n"
                                    "}");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    require_block_concrete(program.m_type_registry, func.m_body);
  }

  TEST_CASE(
      "polymorphic identity: specialization's parameter resolves to i64") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(42)\n"
                                    "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specs = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specs.size() == 1);
    const auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
        specs[0]->m_expression.m_expression);
    REQUIRE(lambda->m_parameters.size() == 1);
    const auto &param_kind =
        program.m_type_registry.get(lambda->m_parameters[0].m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(param_kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(param_kind).m_type ==
          PrimitiveType::I64);
    const auto &ret_kind = program.m_type_registry.get(lambda->m_return_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(ret_kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(ret_kind).m_type ==
          PrimitiveType::I64);
  }

  TEST_CASE(
      "polymorphic identity: two specializations are each fully concrete") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(42);\n"
                                    "  id(true);\n"
                                    "  0\n"
                                    "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specs = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specs.size() == 2);
    for (const auto *spec : specs) {
      require_expression_concrete(program.m_type_registry, spec->m_expression);
      CHECK(is_concrete(program.m_type_registry, spec->m_variable.m_type));
    }
  }

  TEST_CASE(
      "polymorphic identity: two specializations resolve to i64 and bool") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(42);\n"
                                    "  id(true);\n"
                                    "  0\n"
                                    "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specs = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specs.size() == 2);

    std::set<PrimitiveType> param_types;
    for (const auto *spec : specs) {
      const auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
          spec->m_expression.m_expression);
      REQUIRE(lambda->m_parameters.size() == 1);
      const auto &k =
          program.m_type_registry.get(lambda->m_parameters[0].m_type);
      REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(k));
      param_types.insert(std::get<hir::PrimitiveTypeValue>(k).m_type);
    }
    CHECK(param_types.count(PrimitiveType::I64) == 1);
    CHECK(param_types.count(PrimitiveType::BOOL) == 1);
  }

  TEST_CASE("higher-order apply: full program is fully concrete") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let apply = |f, x| { f(x) };\n"
                                    "  let double_it = |x: i64| { x + x };\n"
                                    "  apply(double_it, 5)\n"
                                    "}");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    require_block_concrete(program.m_type_registry, func.m_body);
  }

  TEST_CASE("higher-order apply: apply specialization's f parameter is a "
            "function type") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let apply = |f, x| { f(x) };\n"
                                    "  let double_it = |x: i64| { x + x };\n"
                                    "  apply(double_it, 5)\n"
                                    "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specs = find_let_bindings_with_prefix(func.m_body, "apply");
    REQUIRE(specs.size() == 1);
    const auto &lambda = std::get<std::unique_ptr<hir::LambdaExpr>>(
        specs[0]->m_expression.m_expression);
    REQUIRE(lambda->m_parameters.size() == 2);
    const auto &f_kind =
        program.m_type_registry.get(lambda->m_parameters[0].m_type);
    CHECK(std::holds_alternative<hir::FunctionType>(f_kind));
  }

  TEST_CASE("polymorphic identity used twice at same type: single "
            "specialization, fully concrete") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(1);\n"
                                    "  id(2)\n"
                                    "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    auto specs = find_let_bindings_with_prefix(func.m_body, "id");
    REQUIRE(specs.size() == 1);
    require_block_concrete(program.m_type_registry, func.m_body);
  }

  TEST_CASE(
      "call-site identifiers in polymorphic program have concrete types") {
    // Each rewritten call-site identifier must have a concrete type matching
    // its specialization's function type.
    auto program = mono_zonk_string("fn main() -> i64 {\n"
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
    CHECK(is_concrete(program.m_type_registry, callee_id.m_type));
    const auto &callee_kind = program.m_type_registry.get(callee_id.m_type);
    CHECK(std::holds_alternative<hir::FunctionType>(callee_kind));
  }

  TEST_CASE("nested polymorphic call: id(id)(42) is fully concrete") {
    // `id` applied to itself then to an i64: the outer specialization's
    // parameter is (fn(i64) -> i64), the inner specialization is fn(i64) ->
    // i64.
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(id)(42)\n"
                                    "}");
    REQUIRE(program.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    require_block_concrete(program.m_type_registry, func.m_body);
  }

  TEST_CASE("polymorphic program retains no unifier state after zonking") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(42)\n"
                                    "}");
    CHECK_FALSE(program.m_unifier_state.has_value());
  }

  TEST_CASE("polymorphic program: final expression type matches function "
            "return type") {
    auto program = mono_zonk_string("fn main() -> i64 {\n"
                                    "  let id = |x| { x };\n"
                                    "  id(42)\n"
                                    "}");
    auto &func = std::get<hir::FunctionDef>(program.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(func.m_body.m_final_expression.value().m_type == func.m_body.m_type);
    const auto &kind = program.m_type_registry.get(func.m_body.m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(kind).m_type == PrimitiveType::I64);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
